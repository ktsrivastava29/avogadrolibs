/* This source file is part of the Avogadro project.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "coloropacitymap.h"

#include "comdialog.h"
#include "computehistogram.h"
#include "histogramwidget.h"

#include <QAction>
#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QDebug>

#include <avogadro/core/crystaltools.h>
#include <avogadro/core/cube.h>
#include <avogadro/core/unitcell.h>
#include <avogadro/qtgui/molecule.h>
#include <avogadro/qtopengl/activeobjects.h>
#include <avogadro/qtopengl/glwidget.h>
#include <avogadro/vtk/vtkglwidget.h>
#include <avogadro/vtk/vtkplot.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>

using Avogadro::QtGui::Molecule;
using Avogadro::QtOpenGL::ActiveObjects;

namespace Avogadro::QtPlugins {

vtkImageData* cubeImageData(Core::Cube* cube)
{
  auto data = vtkImageData::New();
  // data->SetNumberOfScalarComponents(1, nullptr);
  Eigen::Vector3i dim = cube->dimensions();
  data->SetExtent(0, dim.x() - 1, 0, dim.y() - 1, 0, dim.z() - 1);

  // Translate origin, spacing, and types from Avogadro to VTK.
  data->SetOrigin(cube->min().x(), cube->min().y(), cube->min().z());
  data->SetSpacing(cube->spacing().data());
  data->AllocateScalars(VTK_FLOAT, 1);

  auto* dataPtr = static_cast<float*>(data->GetScalarPointer());
  std::vector<float>* cubePtr = cube->data();

  // Reorder our cube for VTK's Fortran ordering in vtkImageData.
  for (int i = 0; i < dim.x(); ++i) {
    for (int j = 0; j < dim.y(); ++j) {
      for (int k = 0; k < dim.z(); ++k) {
        dataPtr[(k * dim.y() + j) * dim.x() + i] =
          (*cubePtr)[(i * dim.y() + j) * dim.z() + k];
      }
    }
  }

  return data;
}

ColorOpacityMap::ColorOpacityMap(QObject* p)
  : Avogadro::QtGui::ExtensionPlugin(p), m_actions(QList<QAction*>()),
    m_displayDialogAction(new QAction(this))
{
  m_displayDialogAction->setText(tr("Edit Color Opacity Map…"));
  connect(m_displayDialogAction.data(), &QAction::triggered, this,
          &ColorOpacityMap::displayDialog);
  m_actions.push_back(m_displayDialogAction.data());
  m_displayDialogAction->setProperty("menu priority", 70);

  updateActions();
}

ColorOpacityMap::~ColorOpacityMap() = default;

QList<QAction*> ColorOpacityMap::actions() const
{
  return m_actions;
}

QStringList ColorOpacityMap::menuPath(QAction*) const
{
  return QStringList() << tr("&Extensions");
}

void ColorOpacityMap::setMolecule(QtGui::Molecule* mol)
{
  if (m_molecule == mol)
    return;

  if (m_molecule)
    m_molecule->disconnect(this);

  m_molecule = mol;

  if (m_molecule)
    connect(m_molecule, SIGNAL(changed(uint)), SLOT(moleculeChanged(uint)));

  updateActions();
}

void ColorOpacityMap::moleculeChanged(unsigned int c)
{
  Q_ASSERT(m_molecule == qobject_cast<Molecule*>(sender()));
  // Don't attempt to update anything if there is no dialog to update!
  if (!m_comDialog)
    return;

  // I think we need to look at adding cubes to changes, flaky right now.
  auto changes = static_cast<Molecule::MoleculeChanges>(c);
  if (changes & Molecule::Added || changes & Molecule::Removed) {
    updateActions();
    updateHistogram();
  }
}

void ColorOpacityMap::updateActions()
{
  // Disable everything unless we have VTK and a real molecule and cubes
  auto widget = ActiveObjects::instance().activeWidget();
  auto vtkWidget = qobject_cast<VTK::vtkGLWidget*>(widget);

  if (!vtkWidget || !m_molecule || m_molecule->cubeCount() == 0) {
    foreach (QAction* action, m_actions)
      action->setEnabled(false);
    return;
  }

  foreach (QAction* action, m_actions)
    action->setEnabled(true);
}

void ColorOpacityMap::updateHistogram()
{
  auto widget = ActiveObjects::instance().activeWidget();
  auto vtkWidget = qobject_cast<VTK::vtkGLWidget*>(widget);

  if (widget && vtkWidget && widget != m_activeWidget) {
    if (m_activeWidget)
      disconnect(widget, nullptr, this, nullptr);
    connect(widget, SIGNAL(imageDataUpdated()), SLOT(updateHistogram()));
    m_activeWidget = widget;
  }

  if (vtkWidget && m_molecule && m_molecule->cubeCount()) {
    vtkNew<vtkTable> table;
    auto imageData = vtkWidget->imageData();
    auto lut = vtkWidget->lut();
    auto opacity = vtkWidget->opacityFunction();

    m_histogramWidget->setLUT(lut);
    m_histogramWidget->setOpacityFunction(opacity);
    if (imageData) {
      PopulateHistogram(imageData, table);
      m_histogramWidget->setInputData(table, "image_extents", "image_pops");
    }
  }
}

void ColorOpacityMap::displayDialog()
{
  if (!m_comDialog) {
    auto p = qobject_cast<QWidget*>(parent());
    m_comDialog = new ComDialog(p);
    m_comDialog->setMolecule(m_molecule);
    m_histogramWidget = m_comDialog->histogramWidget();
    // m_c->resize(800, 600);
    connect(m_histogramWidget, SIGNAL(colorMapUpdated()), SLOT(render()));
    connect(m_histogramWidget, SIGNAL(opacityChanged()), SLOT(render()));
    connect(m_comDialog, SIGNAL(renderNeeded()), SLOT(render()));
  }
  updateHistogram();
  m_comDialog->show();
}

void ColorOpacityMap::render()
{
  auto widget = ActiveObjects::instance().activeWidget();
  auto vtkWidget = qobject_cast<VTK::vtkGLWidget*>(widget);
  if (vtkWidget) {
    vtkWidget->renderWindow()->Render();
    vtkWidget->update();
  }
}

} // namespace Avogadro::QtPlugins
