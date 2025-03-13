/*
 * Peony-Qt
 *
 * Copyright (C) 2024, KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 * Authors: Meihong He <hemeihong@kylinos.cn>
 *
 */


#include "kt-operation-test.h"

#include <QApplication>
#include <QtTest/QTest>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KTOperationTest kt;
    QTest::qExec(&kt, argc, argv);
    return 0;
}
