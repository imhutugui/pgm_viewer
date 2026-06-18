#include "mainwindow.h"
#include "imageview.h"
#include "ui_mainwindow.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUI();
    createMenuBar();
    createToolBar();
    createStatusBar();
    
    // Load test image if exists
    loadFile("test_image.pgm");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    m_imageView = new ImageView(this);
    setCentralWidget(m_imageView);
    
    // Connect image modified signal
    connect(m_imageView, &ImageView::imageModified, this, [this]() {
        statusBar()->showMessage("Image modified", 2000);
    });
    
    // Enable drag and drop
    m_imageView->setAcceptDrops(true);
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&Open...", this, &MainWindow::openFile, QKeySequence::Open);
    fileMenu->addAction("&Save", this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QMainWindow::close, QKeySequence::Quit);
    
    // Edit menu
    QMenu *editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Reset Image", this, &MainWindow::resetImage, QKeySequence::Refresh);
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About", this, &MainWindow::aboutDialog);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    
    // Brush size slider
    QLabel *brushLabel = new QLabel("Brush:");
    toolBar->addWidget(brushLabel);
    
    QSlider *brushSlider = new QSlider(Qt::Horizontal);
    brushSlider->setMinimum(1);
    brushSlider->setMaximum(50);
    brushSlider->setValue(5);
    brushSlider->setFixedWidth(150);
    toolBar->addWidget(brushSlider);
    
    QLabel *brushValue = new QLabel("5");
    toolBar->addWidget(brushValue);
    
    connect(brushSlider, &QSlider::valueChanged, this, &MainWindow::adjustBrushSize);
    connect(brushSlider, &QSlider::valueChanged, brushValue, [brushValue](int value) {
        brushValue->setText(QString::number(value));
    });
    
    toolBar->addSeparator();
    
    // Reset button
    toolBar->addAction("Reset", this, &MainWindow::resetImage);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::loadFile(const QString &filename)
{
    if (m_imageView->loadPGM(filename)) {
        statusBar()->showMessage(QString("Loaded: %1").arg(filename), 3000);
        setWindowTitle(QString("PGM Viewer - %1").arg(filename));
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to load: %1").arg(filename));
    }
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open PGM File", "", 
                                                     "PGM Files (*.pgm *.Pgm);;All Files (*)");
    if (!filename.isEmpty()) {
        loadFile(filename);
    }
}

void MainWindow::saveFile()
{
    // For simplicity, save as new file
    saveFileAs();
}

void MainWindow::saveFileAs()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save PGM File", "", 
                                                     "PGM Files (*.pgm);;All Files (*)");
    if (!filename.isEmpty()) {
        m_imageView->savePGM(filename);
        statusBar()->showMessage(QString("Saved: %1").arg(filename), 3000);
    }
}

void MainWindow::resetImage()
{
    m_imageView->resetImage();
    statusBar()->showMessage("Image reset", 2000);
}

void MainWindow::adjustBrushSize(int value)
{
    m_imageView->setBrushSize(value);
}

void MainWindow::aboutDialog()
{
    QMessageBox::about(this, "About PGM Viewer",
        "<h2>PGM Viewer</h2>"
        "<p>A simple PGM image viewer with pixel erase functionality.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>View PGM files (P2/P5 format)</li>"
        "<li>Erase pixels with adjustable brush size</li>"
        "<li>Zoom with mouse wheel (0.1x - 10x)</li>"
        "<li>Pan with middle mouse button or Ctrl+drag</li>"
        "<li>Save modified images</li>"
        "</ul>"
        "<p><b>Controls:</b></p>"
        "<ul>"
        "<li>Left click/drag: Erase pixels</li>"
        "<li>Middle click/drag or Ctrl+drag: Pan view</li>"
        "<li>Mouse wheel: Zoom in/out</li>"
        "</ul>"
        "<p>Built with Qt6</p>");
}
