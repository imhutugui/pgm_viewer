#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <vector>

class ImageView : public QWidget
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);
    ~ImageView();

    bool loadPGM(const QString &filename);
    void savePGM(const QString &filename);
    void resetImage();
    void setBrushSize(int size);
    int brushSize() const;

signals:
    void imageModified();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void erasePixel(int x, int y);
    QPoint screenToImage(const QPoint &screenPos);

    QImage m_originalImage;
    QImage m_displayImage;
    std::vector<std::vector<unsigned char>> m_pixelData;
    int m_width;
    int m_height;
    int m_maxVal;
    
    double m_zoom;
    QPoint m_offset;
    int m_brushSize;
    bool m_isDrawing;
    QPoint m_lastPanPos;  // For pan operation
    QPoint m_currentMousePos;  // For brush preview
};

#endif // IMAGEVIEW_H
