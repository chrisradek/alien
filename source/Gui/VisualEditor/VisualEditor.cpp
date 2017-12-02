#include <QScrollBar>
#include <QTimer>
#include <QGraphicsItem>

#include "Gui/Settings.h"
#include "Model/Api/SimulationAccess.h"
#include "Model/Local/UnitContext.h"
#include "Model/Local/SpaceMetricLocal.h"
#include "PixelUniverseView.h"
#include "ItemUniverseView.h"
#include "ViewportController.h"

#include "VisualEditor.h"
#include "ui_VisualEditor.h"


VisualEditor::VisualEditor(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::VisualEditor)
	, _pixelUniverse(new PixelUniverseView(this))
	, _itemUniverse(new ItemUniverseView(this))
	, _viewport(new ViewportController(this))
{
    ui->setupUi(this);

    ui->simulationView->horizontalScrollBar()->setStyleSheet(GuiSettings::ScrollbarStyleSheet);
    ui->simulationView->verticalScrollBar()->setStyleSheet(GuiSettings::ScrollbarStyleSheet);
}

VisualEditor::~VisualEditor()
{
    delete ui;
}

void VisualEditor::init(Notifier* notifier, SimulationController* controller, DataManipulator* manipulator)
{
	_pixelUniverseInit = false;
	_shapeUniverseInit = false;
	_controller = controller;
	_pixelUniverse->init(controller, manipulator, _viewport);
	_itemUniverse->init(notifier, controller, manipulator, _viewport);
	_viewport->init(ui->simulationView, _pixelUniverse, _itemUniverse, ActiveScene::PixelScene);
	setActiveScene(_activeScene);
}

void VisualEditor::refresh()
{
	_pixelUniverse->refresh();
	_itemUniverse->refresh();
}


void VisualEditor::setActiveScene (ActiveScene activeScene)
{
	_viewport->saveScrollPos();
	if (activeScene == ActiveScene::PixelScene) {
		_itemUniverse->deactivate();
	}
	if (activeScene == ActiveScene::ItemScene) {
		_pixelUniverse->deactivate();
	}
	_activeScene = activeScene;
	_viewport->setActiveScene(activeScene);

	if (activeScene == ActiveScene::PixelScene) {
		_pixelUniverse->activate();
	}
	if (activeScene == ActiveScene::ItemScene) {
		_itemUniverse->activate();
	}
	_viewport->restoreScrollPos();
}

QVector2D VisualEditor::getViewCenterWithIncrement ()
{
	QVector2D center = _viewport->getCenter();

    QVector2D posIncrement(_posIncrement, -_posIncrement);
    _posIncrement = _posIncrement + 1.0;
    if( _posIncrement > 9.0)
        _posIncrement = 0.0;
    return center + posIncrement;
}

QGraphicsView* VisualEditor::getGraphicsView ()
{
    return ui->simulationView;
}

qreal VisualEditor::getZoomFactor ()
{
	return _viewport->getZoomFactor();
}

void VisualEditor::zoomIn ()
{
	_viewport->zoomIn();
}

void VisualEditor::zoomOut ()
{
	_viewport->zoomOut();
}



