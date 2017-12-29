/****************************************************************************

 Copyright (C) 2002-2014 Gilles Debunne. All rights reserved.

 This file is part of the QGLViewer library version 2.7.1.

 http://www.libqglviewer.com - contact@libqglviewer.com

 This file may be used under the terms of the GNU General Public License 
 versions 2.0 or 3.0 as published by the Free Software Foundation and
 appearing in the LICENSE file included in the packaging of this file.
 In addition, as a special exception, Gilles Debunne gives you certain 
 additional rights, described in the file GPL_EXCEPTION in this package.

 libQGLViewer uses dual licensing. Commercial/proprietary software must
 purchase a libQGLViewer Commercial License.

 This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

*****************************************************************************/

#include "constrainedCamera.h"

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QKeyEvent>

using namespace qglviewer;
using namespace std;

AxisPlaneConstraint::Type
Viewer::nextTranslationConstraintType(const AxisPlaneConstraint::Type &type) {
  switch (type) {
  case AxisPlaneConstraint::FREE:
    return AxisPlaneConstraint::PLANE;
    break;
  case AxisPlaneConstraint::PLANE:
    return AxisPlaneConstraint::AXIS;
    break;
  case AxisPlaneConstraint::AXIS:
    return AxisPlaneConstraint::FORBIDDEN;
    break;
  case AxisPlaneConstraint::FORBIDDEN:
    return AxisPlaneConstraint::FREE;
    break;
  default:
    return AxisPlaneConstraint::FREE;
  }
}

AxisPlaneConstraint::Type
Viewer::nextRotationConstraintType(const AxisPlaneConstraint::Type &type) {
  switch (type) {
  case AxisPlaneConstraint::FREE:
    return AxisPlaneConstraint::AXIS;
    break;
  case AxisPlaneConstraint::PLANE:
    return AxisPlaneConstraint::FREE;
    break;
  case AxisPlaneConstraint::AXIS:
    return AxisPlaneConstraint::FORBIDDEN;
    break;
  case AxisPlaneConstraint::FORBIDDEN:
    return AxisPlaneConstraint::FREE;
    break;
  default:
    return AxisPlaneConstraint::FREE;
  }
}

void Viewer::changeConstraint() {
  unsigned short previous = activeConstraint;
  activeConstraint = (activeConstraint + 1) % 2;

  //设置平移限制的格式(平移矩阵)
  constraints[activeConstraint]->setTranslationConstraintType(
      constraints[previous]->translationConstraintType());
  //设置平移矩阵的方向
  constraints[activeConstraint]->setTranslationConstraintDirection(
      constraints[previous]->translationConstraintDirection());
  //设置旋转限制的格式(旋转矩阵)
  constraints[activeConstraint]->setRotationConstraintType(
      constraints[previous]->rotationConstraintType());
  //设置旋转矩阵的方向
  constraints[activeConstraint]->setRotationConstraintDirection(
      constraints[previous]->rotationConstraintDirection());

  //把设置好的平移矩阵和旋转矩阵给摄像机变量
  camera()->frame()->setConstraint(constraints[activeConstraint]);
}

void Viewer::init() {
  restoreStateFromFile();//刷新viewer界面

  constraints[0] = new WorldConstraint();

  // Note that a CameraConstraint(camera) would produce the same results:
  // A CameraConstraint is a LocalConstraint when applied to the camera frame !
  constraints[1] = new LocalConstraint();

  transDir = 0;
  rotDir = 0;
  activeConstraint = 0;

  camera()->frame()->setConstraint(constraints[activeConstraint]);

  setAxisIsDrawn();//绘制轴

  setKeyDescription(Qt::Key_G, "Change translation constraint direction");
  setKeyDescription(Qt::Key_D, "Change rotation constraint direction");
  setKeyDescription(Qt::Key_Space, "Change constraint reference");
  setKeyDescription(Qt::Key_T, "Change translation constraint type");
  setKeyDescription(Qt::Key_R, "Change rotation constraint type");

  help();
}

void Viewer::draw() {
  const float nbSteps = 200.0;
  glBegin(GL_QUAD_STRIP);
  for (float i = 0; i < nbSteps; ++i) {
    float ratio = i / nbSteps;
    float angle = 21.0 * ratio;
    float c = cos(angle);
    float s = sin(angle);
    float r1 = 1.0 - 0.8 * ratio;
    float r2 = 0.8 - 0.8 * ratio;
    float alt = ratio - 0.5;
    const float nor = .5;
    const float up = sqrt(1.0 - nor * nor);
    glColor3f(1 - ratio, 0.2f, ratio);
    glNormal3f(nor * c, up, nor * s);
    glVertex3f(r1 * c, alt, r1 * s);
    glVertex3f(r2 * c, alt + 0.05, r2 * s);
  }
  glEnd();

  displayText();
}

void Viewer::keyPressEvent(QKeyEvent *e) {//键盘事件
  switch (e->key()) {
  case Qt::Key_G:
    transDir = (transDir + 1) % 3;//控制平移轴
    break;
  case Qt::Key_D:
    rotDir = (rotDir + 1) % 3;//控制旋转轴
    break;
  case Qt::Key_Space:
    changeConstraint();//控制限制(把矩阵设置给相机)
    break;
  case Qt::Key_T://设置平移矩阵的格式
    constraints[activeConstraint]->setTranslationConstraintType(
        nextTranslationConstraintType(
            constraints[activeConstraint]->translationConstraintType()));
    break;
  case Qt::Key_R://设置旋转矩阵的格式
    constraints[activeConstraint]->setRotationConstraintType(
        nextRotationConstraintType(
            constraints[activeConstraint]->rotationConstraintType()));
    break;
  default://除以上情况需响应键盘的其他事件
    QGLViewer::keyPressEvent(e);
  }

  Vec dir(0.0, 0.0, 0.0);
  dir[transDir] = 1.0;//当transDir为0时，沿着x轴平移，并使用这个坐标设置矩阵
  constraints[activeConstraint]->setTranslationConstraintDirection(dir);

  dir = Vec(0.0, 0.0, 0.0);
  dir[rotDir] = 1.0;//当transDir为0时，沿着x轴旋转，并使用这个坐标设置矩阵
  constraints[activeConstraint]->setRotationConstraintDirection(dir);

  update();//更新矩阵
}

void Viewer::displayType(const AxisPlaneConstraint::Type type, const int x,
                         const int y, const char c) {
  QString text;
  switch (type) {
  case AxisPlaneConstraint::FREE:
    text = QString("FREE (%1)").arg(c);
    break;
  case AxisPlaneConstraint::PLANE:
    text = QString("PLANE (%1)").arg(c);
    break;
  case AxisPlaneConstraint::AXIS:
    text = QString("AXIS (%1)").arg(c);
    break;
  case AxisPlaneConstraint::FORBIDDEN:
    text = QString("FORBIDDEN (%1)").arg(c);
    break;
  }
  drawText(x, y, text);
}

void Viewer::displayDir(const unsigned short dir, const int x, const int y,
                        const char c) {
  QString text;
  switch (dir) {
  case 0:
    text = QString("X (%1)").arg(c);
    break;
  case 1:
    text = QString("Y (%1)").arg(c);
    break;
  case 2:
    text = QString("Z (%1)").arg(c);
    break;
  }
  drawText(x, y, text);
}

void Viewer::displayText() {
    //设置viewer的文字颜色
  glColor4f(foregroundColor().redF(), foregroundColor().greenF(),
            foregroundColor().blueF(), foregroundColor().alphaF());
  glDisable(GL_LIGHTING);
  //绘制文字 参数：位置x，y值，文字内容
  drawText(10, height() - 30, "TRANSLATION :");
  //绘制文字 参数：由于键盘事件，当点击‘G’时，判断显示平移的轴；
  displayDir(transDir, 190, height() - 30, 'G');
  //绘制文字 参数：由于键盘事件，当点击‘T’时，判断显示矩阵的格式；
  displayType(constraints[activeConstraint]->translationConstraintType(), 10,
              height() - 60, 'T');

  //绘制文字
  drawText(width() - 220, height() - 30, "ROTATION :");
  //绘制文字 按哪个轴进行旋转
  displayDir(rotDir, width() - 100, height() - 30, 'D');
  displayType(constraints[activeConstraint]->rotationConstraintType(),
              width() - 220, height() - 60, 'R');

  //根据限制进行显示
  switch (activeConstraint) {
  case 0:
    drawText(20, 20, "Constraint direction defined w/r to WORLD (SPACE)");
    break;
  case 1:
    drawText(20, 20, "Constraint direction defined w/r to CAMERA (SPACE)");
    break;
  }

  glEnable(GL_LIGHTING);
}

QString Viewer::helpString() const {
  QString text("<h2>C o n s t r a i n e d C a m e r a</h2>");
  text += "The camera frame can be constrained to limit the camera "
          "displacements.<br><br>";
  text += "Try the different translation (press <b>G</b> and <b>T</b>) and "
          "rotation ";
  text += "(<b>D</b> and <b>R</b>) constraints while moving the camera with "
          "the mouse. ";
  text += "The constraints can be defined with respect to various coordinates ";
  text += "systems : press <b>Space</b> to switch.<br><br>";
  text += "You can easily define your own constraints to create a specific "
          "camera constraint.";
  return text;
}
