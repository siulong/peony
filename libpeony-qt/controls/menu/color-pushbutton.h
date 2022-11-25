#ifndef COLORPUSHBUTTON_H
#define COLORPUSHBUTTON_H

#include <QWidget>
#include <QPushButton>
#include <QPainter>
#include <QColor>
#include <QEnterEvent>
class ColorPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ColorPushButton(QColor color,QWidget *parent = nullptr);


//Q_SIGNALS:
protected:
    void paintEvent(QPaintEvent *e);

private:
    QColor m_color = nullptr;
    QRectF m_pathrect;
    double m_pathrectwidth;
    double m_pathrectheigth;
    QRectF m_bigpathrect;
    double m_bigpathrectwidth;
    double m_bigpathrectheigth;
};

#endif // COLORPUSHBUTTON_H
