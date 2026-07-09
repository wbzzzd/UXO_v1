/**
 * @file AirportData.cpp
 * @brief 机场数据实现
 * @details 实现机场数据结构及默认数据加载功能
 */
#include "Core/Data/AirportData.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

namespace Core {

AirportData::AirportData()
    : id("airport_default")
    , name("模拟机场")
    , version("1.0.0")
    , cameraDistance(800.0f)
    , cameraTarget(1500.0f, 0.0f, 1500.0f)
{
    loadDefaultData();
}

void AirportData::loadDefaultData()
{
    // 地面
    ground = GroundInfo();
    ground.position = QVector3D(2000.0f, -1.0f, 2000.0f);
    ground.xExtent = 5000.0f;
    ground.zExtent = 5000.0f;
    ground.yExtent = 2.0f;
    ground.color = "#4A7C3F";

    // 跑道
    runways.clear();
    RunwayInfo runway;
    runway.position = QVector3D(1500.0f, 0.0f, 1500.0f);
    runway.length = 3000.0f;
    runway.width = 50.0f;
    runway.thickness = 0.3f;
    runway.direction = QVector3D(1.0f, 0.0f, 0.0f);
    runway.centerlineColor = "#FFD700";
    runway.centerlineThickness = 0.05f;
    runways.append(runway);

    // 滑行道
    taxiways.clear();
    TaxiwayInfo taxiway;
    taxiway.position = QVector3D(800.0f, 0.0f, 1700.0f);
    taxiway.length = 600.0f;
    taxiway.width = 200.0f;
    taxiway.thickness = 0.25f;
    taxiway.name = "A";
    taxiways.append(taxiway);

    // 停机坪
    aprons.clear();
    ApronInfo apron;
    apron.position = QVector3D(300.0f, 0.0f, 1800.0f);
    apron.length = 400.0f;
    apron.width = 400.0f;
    apron.thickness = 0.2f;
    apron.parkingCount = 8;
    aprons.append(apron);

    // 光照
    lights.clear();
    LightInfo light1;
    light1.position = QVector3D(1500.0f, 500.0f, 1500.0f);
    light1.color = "#FFFFFF";
    light1.intensity = 1.0f;
    light1.attenuation = 0.001f;
    lights.append(light1);

    LightInfo light2;
    light2.position = QVector3D(3000.0f, 300.0f, 3000.0f);
    light2.color = "#FFFFFF";
    light2.intensity = 0.5f;
    light2.attenuation = 0.0f;
    lights.append(light2);

    // 默认相机参数
    cameraDistance = 800.0f;
    cameraTarget = QVector3D(1500.0f, 0.0f, 1500.0f);
}

bool AirportData::loadFromJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开机场数据文件:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "机场数据JSON格式错误:" << filePath;
        return false;
    }

    QJsonObject root = doc.object();
    id = root.value("id").toString("airport_default");
    name = root.value("name").toString("模拟机场");
    version = root.value("version").toString("1.0.0");

    // 解析地面
    if (root.contains("ground")) {
        QJsonObject groundObj = root["ground"].toObject();
        ground.position.setX(groundObj.value("x").toDouble(2000.0));
        ground.position.setY(groundObj.value("y").toDouble(-1.0));
        ground.position.setZ(groundObj.value("z").toDouble(2000.0));
        ground.xExtent = groundObj.value("xExtent").toDouble(5000.0);
        ground.zExtent = groundObj.value("zExtent").toDouble(5000.0);
        ground.yExtent = groundObj.value("yExtent").toDouble(2.0);
        ground.color = groundObj.value("color").toString("#4A7C3F");
    }

    // 解析跑道
    runways.clear();
    if (root.contains("runways")) {
        QJsonArray runwayArray = root["runways"].toArray();
        for (int i = 0; i < runwayArray.size(); ++i) {
            QJsonObject obj = runwayArray[i].toObject();
            RunwayInfo runway;
            runway.position.setX(obj.value("x").toDouble(1500.0));
            runway.position.setY(obj.value("y").toDouble(0.0));
            runway.position.setZ(obj.value("z").toDouble(1500.0));
            runway.length = obj.value("length").toDouble(3000.0);
            runway.width = obj.value("width").toDouble(50.0);
            runway.thickness = obj.value("thickness").toDouble(0.3);
            runway.centerlineColor = obj.value("centerlineColor").toString("#FFD700");
            runway.centerlineThickness = obj.value("centerlineThickness").toDouble(0.05);
            runways.append(runway);
        }
    }

    // 解析滑行道
    taxiways.clear();
    if (root.contains("taxiways")) {
        QJsonArray taxiwayArray = root["taxiways"].toArray();
        for (int i = 0; i < taxiwayArray.size(); ++i) {
            QJsonObject obj = taxiwayArray[i].toObject();
            TaxiwayInfo taxiway;
            taxiway.position.setX(obj.value("x").toDouble(800.0));
            taxiway.position.setY(obj.value("y").toDouble(0.0));
            taxiway.position.setZ(obj.value("z").toDouble(1700.0));
            taxiway.length = obj.value("length").toDouble(600.0);
            taxiway.width = obj.value("width").toDouble(200.0);
            taxiway.thickness = obj.value("thickness").toDouble(0.25);
            taxiway.name = obj.value("name").toString("A");
            taxiways.append(taxiway);
        }
    }

    // 解析停机坪
    aprons.clear();
    if (root.contains("aprons")) {
        QJsonArray apronArray = root["aprons"].toArray();
        for (int i = 0; i < apronArray.size(); ++i) {
            QJsonObject obj = apronArray[i].toObject();
            ApronInfo apron;
            apron.position.setX(obj.value("x").toDouble(300.0));
            apron.position.setY(obj.value("y").toDouble(0.0));
            apron.position.setZ(obj.value("z").toDouble(1800.0));
            apron.length = obj.value("length").toDouble(400.0);
            apron.width = obj.value("width").toDouble(400.0);
            apron.thickness = obj.value("thickness").toDouble(0.2);
            apron.parkingCount = obj.value("parkingCount").toInt(8);
            aprons.append(apron);
        }
    }

    // 解析光照
    lights.clear();
    if (root.contains("lights")) {
        QJsonArray lightArray = root["lights"].toArray();
        for (int i = 0; i < lightArray.size(); ++i) {
            QJsonObject obj = lightArray[i].toObject();
            LightInfo light;
            light.position.setX(obj.value("x").toDouble(1500.0));
            light.position.setY(obj.value("y").toDouble(500.0));
            light.position.setZ(obj.value("z").toDouble(1500.0));
            light.color = obj.value("color").toString("#FFFFFF");
            light.intensity = obj.value("intensity").toDouble(1.0);
            light.attenuation = obj.value("attenuation").toDouble(0.001);
            lights.append(light);
        }
    }

    qInfo() << "机场数据加载成功:" << name << "版本:" << version;
    return true;
}

bool AirportData::saveToJson(const QString& filePath) const
{
    QJsonObject root;
    root["id"] = id;
    root["name"] = name;
    root["version"] = version;

    // 地面
    QJsonObject groundObj;
    groundObj["x"] = ground.position.x();
    groundObj["y"] = ground.position.y();
    groundObj["z"] = ground.position.z();
    groundObj["xExtent"] = ground.xExtent;
    groundObj["zExtent"] = ground.zExtent;
    groundObj["yExtent"] = ground.yExtent;
    groundObj["color"] = ground.color;
    root["ground"] = groundObj;

    // 跑道
    QJsonArray runwayArray;
    for (const auto& runway : runways) {
        QJsonObject obj;
        obj["x"] = runway.position.x();
        obj["y"] = runway.position.y();
        obj["z"] = runway.position.z();
        obj["length"] = runway.length;
        obj["width"] = runway.width;
        obj["thickness"] = runway.thickness;
        obj["centerlineColor"] = runway.centerlineColor;
        obj["centerlineThickness"] = runway.centerlineThickness;
        runwayArray.append(obj);
    }
    root["runways"] = runwayArray;

    // 滑行道
    QJsonArray taxiwayArray;
    for (const auto& taxiway : taxiways) {
        QJsonObject obj;
        obj["x"] = taxiway.position.x();
        obj["y"] = taxiway.position.y();
        obj["z"] = taxiway.position.z();
        obj["length"] = taxiway.length;
        obj["width"] = taxiway.width;
        obj["thickness"] = taxiway.thickness;
        obj["name"] = taxiway.name;
        taxiwayArray.append(obj);
    }
    root["taxiways"] = taxiwayArray;

    // 停机坪
    QJsonArray apronArray;
    for (const auto& apron : aprons) {
        QJsonObject obj;
        obj["x"] = apron.position.x();
        obj["y"] = apron.position.y();
        obj["z"] = apron.position.z();
        obj["length"] = apron.length;
        obj["width"] = apron.width;
        obj["thickness"] = apron.thickness;
        obj["parkingCount"] = apron.parkingCount;
        apronArray.append(obj);
    }
    root["aprons"] = apronArray;

    // 光照
    QJsonArray lightArray;
    for (const auto& light : lights) {
        QJsonObject obj;
        obj["x"] = light.position.x();
        obj["y"] = light.position.y();
        obj["z"] = light.position.z();
        obj["color"] = light.color;
        obj["intensity"] = light.intensity;
        obj["attenuation"] = light.attenuation;
        lightArray.append(obj);
    }
    root["lights"] = lightArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法保存机场数据文件:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qInfo() << "机场数据已保存:" << filePath;
    return true;
}

} // namespace Core