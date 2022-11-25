#include "color-pushbutton.h"
#include <QPainter>
#include <QDebug>

#include <QPainterPath>

ColorPushButton::ColorPushButton(QColor color,QWidget *parent) : QPushButton(parent)
{
    m_color = color;
    setProperty("isRoundButton",true);
    setFixedSize(12,12);
    QRectF rect = this->rect();
    m_pathrect = rect.adjusted(1, 1, -1, -1);
    m_pathrectwidth =m_pathrect.width();
    m_pathrectheigth = m_pathrect.height();
    QSizeF size(18,18);
    rect.setSize(size);
    m_bigpathrect = rect.adjusted(1,1,-1,-1);
    m_bigpathrectwidth = m_bigpathrect.width();
    m_bigpathrectheigth = m_bigpathrect.height();

}

void ColorPushButton::paintEvent(QPaintEvent *e)
{
    if(this->isChecked()){
        if(this->underMouse()){
            QPalette pal = this->palette();
            pal.setColor(QPalette::Active,QPalette::Highlight, m_color);
            setFixedSize(18,18);
            this->setPalette(pal);
            QPushButton::paintEvent(e);

            QPainter painter(this);
            painter.save();
            painter.setPen(Qt::white);
            QPainterPath path;
            path.moveTo(m_bigpathrect.left() + m_bigpathrectwidth / 4, m_bigpathrect.top() + m_bigpathrectheigth / 4);
            path.lineTo(m_bigpathrect.left() + m_bigpathrectwidth * 0.75, m_bigpathrect.top() + m_bigpathrectheigth *0.75);
            path.moveTo(m_bigpathrect.left() + m_bigpathrectwidth / 4, m_bigpathrect.top() + m_bigpathrectheigth *0.75);
            path.lineTo(m_bigpathrect.left() + m_bigpathrectwidth * 0.75, m_bigpathrect.top() + m_bigpathrectheigth / 4);
            painter.drawPath(path);
            painter.restore();
        }else{
            QPalette pal = this->palette();
            pal.setColor(QPalette::Active,QPalette::Highlight, m_color);
            this->setPalette(pal);
            setFixedSize(12,12);
            QPushButton::paintEvent(e);
            QPainter painter(this);
            painter.save();

            painter.setPen(Qt::white);
            QPainterPath path;
            path.moveTo(m_pathrect.left() + m_pathrectwidth / 4, m_pathrect.top() + m_pathrectheigth / 2);
            path.lineTo(m_pathrect.left() + m_pathrectwidth * 0.45, m_pathrect.bottom() - m_pathrectheigth / 3);
            path.lineTo(m_pathrect.right() - m_pathrectwidth / 4, m_pathrect.top() + m_pathrectheigth / 3);
            painter.drawPath(path);
            painter.restore();
        }
    }else{
        if(this->underMouse()){
            QPalette pal = this->palette();
            pal.setColor(QPalette::Active,QPalette::Highlight, m_color);
            this->setPalette(pal);
            setFixedSize(18,18);
            QPushButton::paintEvent(e);

            QPainter painter(this);
            painter.save();
            painter.setPen(Qt::white);
            QPainterPath path;
            path.moveTo(m_bigpathrect.left() + m_bigpathrectwidth / 4, m_bigpathrect.top() + m_bigpathrectheigth / 2);
            path.lineTo(m_bigpathrect.left() + m_bigpathrectwidth * 0.80, m_bigpathrect.top() + m_bigpathrectheigth / 2);
            path.moveTo(m_bigpathrect.left() + m_bigpathrectwidth / 2, m_bigpathrect.top() + m_bigpathrectheigth / 4);
            path.lineTo(m_bigpathrect.left() + m_bigpathrectwidth / 2, m_bigpathrect.top() + m_bigpathrectheigth * 0.80);
            painter.drawPath(path);
            painter.restore();
        }else if(!this->underMouse()){
            QPalette pal = this->palette();
            pal.setColor(QPalette::Button, m_color);
            this->setPalette(pal);
            setFixedSize(12,12);
            QPushButton::paintEvent(e);
            }
    }

}
