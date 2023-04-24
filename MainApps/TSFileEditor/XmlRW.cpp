#include "XmlRW.h"

#include <QDebug>
#include <QDomNodeList>

#include "log.h"

#define ROOT_ELEMENT        "TS"
#define CONTEXT_ELEMENT     "context"
#define MESSAGE_ELEMENT     "message"
#define NAME_ELEMENT     "name"
#define LOCATION_ELEMENT    "location"
#define SOURCE_ELEMENT      "source"
#define TRANSLATION_ELEMENT "translation"

XmlRW::XmlRW(QObject *parent) : QObject(parent)
{

}

void XmlRW::UpdateTranslateMap(QList<TranslateModel>& list)
{
    jsonArray = QJsonArray();

    foreach (TranslateModel model, list) {
        QJsonObject messageObject;
        messageObject.insert("id", "");
        messageObject.insert("filename", model.GetKey());
        messageObject.insert("source", model.GetSource());
        messageObject.insert("translationType", model.GetTranslate());
        jsonArray.append(messageObject);QLOG(model.GetKey() << model.GetSource() << model.GetTranslate());

        //m_translateMap.insert(model.GetKey(), {model.GetSource(), model.GetTranslate()});
    }
    rootObject.insert("messages", jsonArray);

//    m_translateMap.clear();

//    foreach (TranslateModel model, list) {
//       m_translateMap.insert(model.GetKey(), {model.GetSource(), model.GetTranslate()});
//       QLOG(model.GetKey() << model.GetSource() << model.GetTranslate());
//    }
}

bool XmlRW::ImportFromTS(QList<TranslateModel>& list, QString strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    else {
        xml.setDevice(&file);
//        m_translateMap.clear();

        if (xml.readNextStartElement()) {
            QString strName = xml.name().toString();QLOG(strName);
            if (strName== ROOT_ELEMENT) {
                QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("version")) {
                    QString strVersion = attributes.value("version").toString();
                    qDebug() << "version : " << strVersion;
                }
                if (attributes.hasAttribute("language")) {
                    QString strLanguage = attributes.value("language").toString();
                    qDebug() << "language : " << strLanguage;
                }

                ReadXBEL();
            } else {
                xml.raiseError("XML file format error.");
            }
        }

        file.close();
        list.clear();

//        for (auto i = m_translateMap.begin(); i != m_translateMap.end(); ++i) {
//            TranslateModel model;
//            model.SetKey(i.key());
//            model.SetSource(i.value().at(0));
//            model.SetTranslate(i.value().at(1));QLOG(i.key() << i.value());
//            list.append(model);
//        }


        for (int i = 0; i < jsonArray.size(); i++) {
            QJsonObject messageObject = jsonArray.at(i).toObject();
            QString filename = messageObject.value("filename").toString();
            QString messageId = messageObject.value("id").toString();
            QString source = messageObject.value("source").toString();
            QString translationType = messageObject.value("translation").toString();

            TranslateModel model;
            model.SetKey(filename);
            model.SetSource(source);
            model.SetTranslate(translationType);//QLOG(filename << source << translationType);
            list.append(model);
        }
        return true;
    }
}

bool XmlRW::ExportToTS(QList<TranslateModel>& modeList, QString strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    else {
        if(modeList.count() <=0) {
            qDebug() << "translate modeList is empty";
            return false;
        }

        UpdateTranslateMap(modeList);

        QDomDocument doc;
        if(!doc.setContent(&file)) // 打开 ts 文件
        {
            qDebug() << "xml parsing error";
            return false;
        }
        file.close();QLOG(file);

        QDomElement root=doc.documentElement();
        QDomNodeList list=root.elementsByTagName("message");

//        QLOG(list.count());

        QDomNode node;
        for(int i=0; i < list.count(); i++) {
            node = list.at(i);
            QDomNodeList childList = node.childNodes();//QLOG(childList.count());

            for (int i = 0; i < childList.length(); i++) {
                QDomNode childNode = childList.item(i);
                QLOG(childNode.toElement().text());
            }

            QString strKey = childList.at(childList.count()-2).toElement().text();QLOG(childList.at(1).toElement().text() << childList.at(2).toElement().text());
            QString strTranslation = node.lastChild().toElement().text();//QLOG(node.lastChild().toElement().text()<<m_translateMap.value(strKey));

//            QString strValue = m_translateMap.value(strKey).value(1);QLOG(m_translateMap.value(strKey));
            QString strValue;
            for (int i = 0; i < jsonArray.size(); i++) {
                QJsonObject messageObject = jsonArray.at(i).toObject();
                QString filename = messageObject.value("filename").toString();
                QString translationType = messageObject.value("translation").toString();
                if (filename == strKey && translationType == strTranslation) {
                    strValue = messageObject.value("source").toString();
                    break;
                }
            }

            QString translateStr;
            if(i < modeList.count())
                translateStr = modeList.at(i).GetTranslate(); // 使用 excel 中的翻译
            QLOG(i << strKey << strValue << strTranslation << translateStr);

//            qDebug() << i << "\ttranslatation:" << strTranslation
//                     << "\t\tkey:" << strKey << "\t\tvalue:" << strValue;

//            if(!strValue.isEmpty() && strTranslation != strValue) {
            if(!translateStr.isEmpty() && translateStr != strValue) {
                QDomNode oldNode = node.lastChild();

                QDomElement newElement = doc.createElement("translation");
                QDomText text = doc.createTextNode(translateStr);
                newElement.appendChild(text);
                node.replaceChild(newElement, oldNode);
            }
        }

        if(!file.open(QFile::WriteOnly|QFile::Truncate)) {
            return false;
        }

        QTextStream outStream(&file);
        doc.save(outStream, 4);
        file.close();

        return true;
    }
}

QString XmlRW::ErrorString() const
{
    return QString("Error:%1  Line:%2  Column:%3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}

void XmlRW::ReadXBEL()
{
    Q_ASSERT(xml.isStartElement() && xml.name().toString() == ROOT_ELEMENT);


    jsonArray = QJsonArray();
    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNextStartElement() && xml.name() == "message") {
            QString messageId = xml.attributes().value("id").toString();
            QString filename;QString line;
            QString source;
            QString translationType;
            QString translationText;
            while (xml.readNextStartElement() && xml.name() != "message") {
                if (xml.name() == "location" && xml.attributes().hasAttribute("filename")) {
                    if(xml.attributes().hasAttribute("line"))
                        line = xml.attributes().value("line").toString();

                    filename = xml.attributes().value("filename").toString();
                    xml.skipCurrentElement();
                } else if (xml.name() == "source") {
                    source = xml.readElementText();
                } else if (xml.name() == "translation") {
                    translationType = xml.attributes().value("type").toString();
                    translationText = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }
            // Do something with the contents of the message element
            // For example, print them to the console

            if(!translationType.compare("unfinished"))
                translationType = "";

//            m_translateMap.insert(filename, {source, translationType});


            // 存储到 json 数据格式中
            // 按照顺序存储
            QJsonObject messageObject;
            messageObject.insert("id", messageId);
            messageObject.insert("filename", filename);
            messageObject.insert("source", source);
            messageObject.insert("translation", translationText);
            jsonArray.append(messageObject);


            // 存储到 json 数据格式中
            // 按照文件树形存储
//            QJsonObject messageObject;
//            messageObject.insert("id", messageId);
//            messageObject.insert("source", source);
//            messageObject.insert("translation", translationType);

//            // 查找是否已经存在该文件名的节点
//            QJsonValue filenameValue = rootObject.value(filename);
//            if (filenameValue.isUndefined()) {
//                // 如果不存在，则创建一个新的节点
//                QJsonArray messageArray;
//                messageArray.append(messageObject);
//                rootObject.insert(filename, messageArray);
//            } else {
//                // 如果已经存在，则将该消息添加到节点的数组中
//                QJsonArray messageArray = filenameValue.toArray();
//                messageArray.append(messageObject);
//                rootObject.insert(filename, messageArray);
//            }

//            if(filename == "commons/comviews/springframe/HDScreen/forms/springframeadjust.ui")
//                QLOG(messageId << filename << line << source << translationText << translationType);
        }
    }

    rootObject.insert("messages", jsonArray);

    QJsonDocument doc(rootObject);
    QString strJson(doc.toJson(QJsonDocument::Indented));
//    qDebug() << strJson;

    // 遍历 rootObject 中的数据
    // 按照 jsonArray.append(messageObject) 的顺序遍历 jsonArray 中的数据
    for (int i = 0; i < jsonArray.size(); i++) {
        QJsonObject messageObject = jsonArray.at(i).toObject();
        QString filename = messageObject.value("filename").toString();
        QString messageId = messageObject.value("id").toString();
        QString source = messageObject.value("source").toString();
        QString translationType = messageObject.value("translation").toString();
//        if("commons/comviews/springframe/HDScreen/forms/springframeadjust.ui" == filename)
        {
            qDebug() << "filename:" << filename;
            qDebug() << "Message ID:" << messageId;
            qDebug() << "Source:" << source;
            qDebug() << "Translation Type:" << translationType;
        }
    }

//    for (auto i = m_translateMap.begin(); i != m_translateMap.end(); ++i) {
//        QLOG(i.key() << i.value());
//    }

//    while (xml.readNextStartElement()) {
//        QLOG(xml.readElementText() << xml.name().toString());
//        if (xml.name().toString() == CONTEXT_ELEMENT) {
//            ReadContext();
//        } else {
//            xml.skipCurrentElement();
//            QLOG(xml.readElementText() << xml.name().toString());
//        }
//    }
}

void XmlRW::ReadContext()
{
    Q_ASSERT(xml.isStartElement() && xml.name().toString() == CONTEXT_ELEMENT);

    while (xml.readNextStartElement()) {

        QLOG(xml.readElementText() << xml.name().toString());

        if (xml.name().toString() == MESSAGE_ELEMENT
            || xml.name().toString() == NAME_ELEMENT) {
            ReadMessage();
        }
        else {
            xml.skipCurrentElement();QLOG("pass: " << xml.readElementText() << xml.name().toString());
        }
    }
}

void XmlRW::ReadMessage()
{
//    Q_ASSERT(xml.isStartElement() && xml.name().toString() == MESSAGE_ELEMENT);

    QString strSource, strTranslation, strLocation;

    while (xml.readNextStartElement()) {

        QLOG(xml.name().toString());

        if (xml.name().toString() == SOURCE_ELEMENT) {
            strSource = xml.readElementText();QLOG(strSource);
        } else if (xml.name().toString() == TRANSLATION_ELEMENT) {
            strTranslation = xml.readElementText();QLOG(strTranslation);
        } else if (xml.name().toString() == LOCATION_ELEMENT) {
            strLocation.clear();

            QXmlStreamAttributes attributes = xml.attributes();//QLOG(attributes);
            if (attributes.hasAttribute("filename")) {
                QString strFileName = attributes.value("filename").toString();
                strLocation.append(QString("fileName: %1; ").arg(strFileName));
            }
            if (attributes.hasAttribute("line")) {
                QString strLine = attributes.value("line").toString();
                strLocation.append(QString("line: %1; ").arg(strLine));
            }

            xml.skipCurrentElement();
        } else if (xml.name().toString() == NAME_ELEMENT) {
            xml.readNext();QLOG(xml.name().toString() << xml.readNext() << "====================");
            xml.skipCurrentElement();
        } else {
            xml.skipCurrentElement();QLOG(xml.name().toString() << xml.readNext() << "====================");
        }
    }

    QLOG("key:" << strSource << "\ttranslation:" << strTranslation);

//    if(m_translateMap.contains(strSource)) {
//        QLOG("repeat key: " << strSource << "translation:" << strLocation);
//    }
//    m_translateMap.insert(strSource, strTranslation);
}
