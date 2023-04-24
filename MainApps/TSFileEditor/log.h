#ifndef LOG_H
#define LOG_H

#include <QtDebug>

#define QLOG(a)		qDebug() << "[QT DBG]" << __FILE__ << "["<< __func__ << __LINE__ << "]" << a


#endif // LOG_H
