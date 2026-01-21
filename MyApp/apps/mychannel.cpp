#include "mychannel.h"

myChannel::myChannel(QObject *parent) : QObject(parent)
{
}

// ---- C++ -> HTML (通过信号触发 JS) ----
void myChannel::setCity(QString city)
{
    emit cityChanged(city);
}

void myChannel::inputChanged_1(QString input)
{
    emit inputChanged_1_sig(input);
}

void myChannel::inputChanged_2(QString input)
{
    emit inputChanged_2_sig(input);
}

void myChannel::startlocation(QString location)
{
    emit startlocation_sig(location);
}

void myChannel::endlocation(QString location)
{
    emit endlocation_sig(location);
}

void myChannel::selectRoute()
{
    emit selectRoute_sig();
}

// ---- HTML -> C++ (处理来自 JS 的调用) ----
void myChannel::cityChangeResult(QString result)
{
    emit setCityLable(result);
}

void myChannel::getAutocomplete_1(QJsonObject result)
{
    emit sendAutocomplete_1(result);
}

void myChannel::getAutocomplete_2(QJsonObject result)
{
    emit sendAutocomplete_2(result);
}
