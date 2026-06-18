#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class ImageView;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void saveFile();
    void saveFileAs();
    void resetImage();
    void adjustBrushSize(int value);
    void aboutDialog();

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void loadFile(const QString &filename);

    Ui::MainWindow *ui;
    ImageView *m_imageView;
};

#endif // MAINWINDOW_H
