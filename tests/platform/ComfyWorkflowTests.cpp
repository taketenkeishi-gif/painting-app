// ComfyWorkflowTests.cpp
// WorkflowDocument / WorkflowBinding のオフラインユニットテスト。
// ComfyUI サーバー不要。

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest/QtTest>

#include "platform/comfy/WorkflowBinding.h"
#include "platform/comfy/WorkflowDocument.h"

using namespace platform::comfy;

// ─────────────────────────────────────────────────────────────────────────────
// テスト用ワークフロー JSON (最小構成)
// ─────────────────────────────────────────────────────────────────────────────
static QJsonObject makeMinimalWorkflow() {
    // Node 1: LoadImage
    QJsonObject n1, i1;
    i1["image"]  = "original.png";
    i1["upload"] = "image";
    n1["class_type"] = "LoadImage";
    n1["inputs"]     = i1;

    // Node 2: CLIPTextEncode (positive)
    QJsonObject n2, i2;
    i2["text"] = "default positive";
    i2["clip"] = QJsonArray{QJsonArray{"checkpoint"}, 1};
    n2["class_type"] = "CLIPTextEncode";
    n2["inputs"]     = i2;

    // Node 3: CLIPTextEncode (negative)
    QJsonObject n3, i3;
    i3["text"] = "default negative";
    i3["clip"] = QJsonArray{QJsonArray{"checkpoint"}, 1};
    n3["class_type"] = "CLIPTextEncode";
    n3["inputs"]     = i3;

    // Node 4: KSampler
    QJsonObject n4, i4;
    i4["seed"]         = 0;
    i4["steps"]        = 20;
    i4["cfg"]          = 7.0;
    i4["sampler_name"] = "euler";
    i4["scheduler"]    = "normal";
    i4["denoise"]      = 1.0;
    n4["class_type"] = "KSampler";
    n4["inputs"]     = i4;

    // Node 5: SaveImage
    QJsonObject n5, i5;
    i5["filename_prefix"] = "output_";
    n5["class_type"] = "SaveImage";
    n5["inputs"]     = i5;

    // Node 6: ControlNetApply
    QJsonObject n6, i6;
    i6["strength"] = 1.0;
    n6["class_type"] = "ControlNetApply";
    n6["inputs"]     = i6;

    return QJsonObject{
        {"1", n1}, {"2", n2}, {"3", n3},
        {"4", n4}, {"5", n5}, {"6", n6},
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowDocument テスト
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowDocumentTest : public QObject {
    Q_OBJECT
private slots:
    void fromJson_valid() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.isValid());
    }

    void nodeIds_returnsAll() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        const QStringList ids = doc.nodeIds();
        QCOMPARE(ids.size(), 6);
    }

    void findNodesByClass_LoadImage() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        const QStringList ids = doc.findNodesByClass("LoadImage");
        QCOMPARE(ids.size(), 1);
        QCOMPARE(ids.first(), QString("1"));
    }

    void findNodesByClass_CLIPTextEncode_twoNodes() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        const QStringList ids = doc.findNodesByClass("CLIPTextEncode");
        QCOMPARE(ids.size(), 2);
    }

    void nodeClass_returnsCorrectType() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QCOMPARE(doc.nodeClass("4"), QString("KSampler"));
    }

    void setInput_patchesDirectly() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.setInput("4", "seed", 12345));
        QCOMPARE(doc.nodeInputs("4").value("seed").toInt(), 12345);
    }

    void setInput_unknownNode_returnsFalse() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(!doc.setInput("999", "key", "val"));
    }

    void toJson_roundTrip() {
        const QJsonObject wf = makeMinimalWorkflow();
        auto doc = WorkflowDocument::fromJson(wf);
        QCOMPARE(doc.toJson(), wf);
    }

    void findMaskSourceNode_returnsUpstreamOfImageToMask() {
        // Node A: LoadImage (mask source)
        QJsonObject nA, iA;
        iA["image"]  = "mask.png";
        nA["class_type"] = "LoadImage";
        nA["inputs"]     = iA;

        // Node B: ImageToMask — inputs.image points to node A
        QJsonObject nB, iB;
        iB["image"]   = QJsonArray{QJsonValue(QString("A")), 0};
        iB["channel"] = "red";
        nB["class_type"] = "ImageToMask";
        nB["inputs"]     = iB;

        // Node C: KSampler (unrelated)
        QJsonObject nC, iC;
        iC["seed"] = 42;
        nC["class_type"] = "KSampler";
        nC["inputs"]     = iC;

        const QJsonObject wf{{"A", nA}, {"B", nB}, {"C", nC}};
        auto doc = WorkflowDocument::fromJson(wf);

        QCOMPARE(doc.findMaskSourceNode(), QString("A"));
    }

    void findMaskSourceNode_noImageToMask_returnsEmpty() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.findMaskSourceNode().isEmpty());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowBinding テスト
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowBindingTest : public QObject {
    Q_OBJECT
private slots:
    void loadImage_patchesFilename() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(WorkflowBinding::loadImage("1", "uploaded.png")));
        QCOMPARE(doc.nodeInputs("1").value("image").toString(),
                 QString("uploaded.png"));
    }

    void saveImage_patchesPrefix() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(WorkflowBinding::saveImage("5", "lpa_result_")));
        QCOMPARE(doc.nodeInputs("5").value("filename_prefix").toString(),
                 QString("lpa_result_"));
    }

    void clipText_patchesText() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(WorkflowBinding::clipText("2", "a cat")));
        QCOMPARE(doc.nodeInputs("2").value("text").toString(),
                 QString("a cat"));
    }

    void kSampler_fluentPatches() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(
            WorkflowBinding::kSampler("4")
                .seed(42)
                .steps(30)
                .cfg(7.5)
                .denoise(0.8)
                .sampler("dpmpp_2m")
                .scheduler("karras")));
        const QJsonObject inp = doc.nodeInputs("4");
        QCOMPARE(inp.value("seed").toInt(),          42);
        QCOMPARE(inp.value("steps").toInt(),         30);
        QCOMPARE(inp.value("cfg").toDouble(),        7.5);
        QCOMPARE(inp.value("denoise").toDouble(),    0.8);
        QCOMPARE(inp.value("sampler_name").toString(), QString("dpmpp_2m"));
        QCOMPARE(inp.value("scheduler").toString(),    QString("karras"));
    }

    void controlNet_patchesStrength() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(
            WorkflowBinding::controlNet("6").strength(0.6)));
        QCOMPARE(doc.nodeInputs("6").value("strength").toDouble(), 0.6);
    }

    void byClass_occurrenceZero_patchesPositive() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(
            WorkflowBinding::byClass("CLIPTextEncode", 0).set("text", "positive text")));
        // node "2" is the first CLIPTextEncode (sorted by numeric id)
        QCOMPARE(doc.nodeInputs("2").value("text").toString(),
                 QString("positive text"));
        // node "3" must remain unchanged
        QCOMPARE(doc.nodeInputs("3").value("text").toString(),
                 QString("default negative"));
    }

    void byClass_occurrenceOne_patchesNegative() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(
            WorkflowBinding::byClass("CLIPTextEncode", 1).set("text", "negative text")));
        QCOMPARE(doc.nodeInputs("3").value("text").toString(),
                 QString("negative text"));
    }

    void byClass_outOfRange_returnsFalse() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(!doc.apply(
            WorkflowBinding::byClass("CLIPTextEncode", 5).set("text", "x")));
    }

    void apply_unknownNode_returnsFalse() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(!doc.apply(WorkflowBinding::loadImage("999", "x.png")));
    }

    void set_escapeHatch_patchesArbitraryKey() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        QVERIFY(doc.apply(
            WorkflowBinding::byClass("KSampler", 0).set("my_custom_key", 99)));
        QCOMPARE(doc.nodeInputs("4").value("my_custom_key").toInt(), 99);
    }

    void multipleBindings_accumulatePatches() {
        auto doc = WorkflowDocument::fromJson(makeMinimalWorkflow());
        doc.apply(WorkflowBinding::loadImage("1", "new_input.png"));
        doc.apply(WorkflowBinding::clipText("2", "positive"));
        doc.apply(WorkflowBinding::clipText("3", "negative"));
        doc.apply(WorkflowBinding::kSampler("4").seed(7).steps(25));
        doc.apply(WorkflowBinding::saveImage("5", "my_output_"));

        QCOMPARE(doc.nodeInputs("1").value("image").toString(),
                 QString("new_input.png"));
        QCOMPARE(doc.nodeInputs("2").value("text").toString(),  QString("positive"));
        QCOMPARE(doc.nodeInputs("3").value("text").toString(),  QString("negative"));
        QCOMPARE(doc.nodeInputs("4").value("seed").toInt(),     7);
        QCOMPARE(doc.nodeInputs("4").value("steps").toInt(),    25);
        QCOMPARE(doc.nodeInputs("5").value("filename_prefix").toString(),
                 QString("my_output_"));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// QTEST_MAIN_IMPL is replaced by a custom main to run both test objects
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int result = 0;

    {
        WorkflowDocumentTest t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        WorkflowBindingTest t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "ComfyWorkflowTests.moc"
