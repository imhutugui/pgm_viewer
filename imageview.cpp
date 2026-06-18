#include "imageview.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QBrush>
#include <QPen>
#include <QMessageBox>
#include <QDebug>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <cmath>

ImageView::ImageView(QWidget *parent)
    : QWidget(parent)
    , m_width(0)
    , m_height(0)
    , m_maxVal(255)
    , m_zoom(1.0)
    , m_brushSize(5)
    , m_isDrawing(false)
    , m_lastPanPos()
    , m_currentMousePos()
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
}

ImageView::~ImageView()
{
}

bool ImageView::loadPGM(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray allData = file.readAll();
    file.close();

    qInfo() << "File size:" << allData.size() << "filename:" << filename;

    if (allData.isEmpty()) {
        qInfo() << "ERROR: File is empty!";
        return false;
    }

    int pos = 0;
    int len = allData.size();

    // Show first 100 bytes for debugging
    QByteArray preview = allData.left(qMin(100, len));
    qInfo() << "File start (hex):" << preview.toHex(':').constData();

    // Skip whitespace
    while (pos < len && (allData[pos] == ' ' || allData[pos] == '\t' || allData[pos] == '\n' || allData[pos] == '\r')) pos++;

    // Read magic number
    QString magic;
    while (pos < len && allData[pos] != ' ' && allData[pos] != '\t' && allData[pos] != '\n' && allData[pos] != '\r') {
        magic += allData[pos++];
    }

    if (magic != "P2" && magic != "P5") {
        return false;
    }

    // Read width (skip comments and whitespace)
    while (pos < len) {
        if (allData[pos] == '#') {
            while (pos < len && allData[pos] != '\n') pos++;
        } else if (allData[pos] == ' ' || allData[pos] == '\t' || allData[pos] == '\n' || allData[pos] == '\r') {
            pos++;
        } else {
            break;
        }
    }
    QString wStr;
    while (pos < len && allData[pos] != ' ' && allData[pos] != '\t' && allData[pos] != '\n' && allData[pos] != '\r') {
        wStr += allData[pos++];
    }
    if (wStr.isEmpty()) return false;
    m_width = wStr.toInt();

    // Read height
    while (pos < len) {
        if (allData[pos] == '#') {
            while (pos < len && allData[pos] != '\n') pos++;
        } else if (allData[pos] == ' ' || allData[pos] == '\t' || allData[pos] == '\n' || allData[pos] == '\r') {
            pos++;
        } else {
            break;
        }
    }
    QString hStr;
    while (pos < len && allData[pos] != ' ' && allData[pos] != '\t' && allData[pos] != '\n' && allData[pos] != '\r') {
        hStr += allData[pos++];
    }
    if (hStr.isEmpty()) return false;
    m_height = hStr.toInt();

    // Read max value
    while (pos < len) {
        if (allData[pos] == '#') {
            while (pos < len && allData[pos] != '\n') pos++;
        } else if (allData[pos] == ' ' || allData[pos] == '\t' || allData[pos] == '\n' || allData[pos] == '\r') {
            pos++;
        } else {
            break;
        }
    }
    QString mStr;
    while (pos < len && allData[pos] != ' ' && allData[pos] != '\t' && allData[pos] != '\n' && allData[pos] != '\r') {
        mStr += allData[pos++];
    }
    if (mStr.isEmpty()) return false;
    m_maxVal = mStr.toInt();

    qInfo() << "Parsed header:" << m_width << "x" << m_height << "maxVal:" << m_maxVal << "dataOffset:" << pos;

    if (m_width <= 0 || m_height <= 0 || m_maxVal <= 0) {
        return false;
    }

    // Initialize pixel data
    m_pixelData.resize(m_height, std::vector<unsigned char>(m_width));

    if (magic == "P5") {
        int expectedPixels = m_width * m_height;
        int availableBytes = allData.size() - pos;

        if (availableBytes < expectedPixels) {
            return false;
        }

        const uchar *rawData = reinterpret_cast<const uchar*>(allData.constData() + pos);
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                m_pixelData[y][x] = rawData[y * m_width + x];
            }
        }
    } else {
        int x = 0, y = 0;
        while (y < m_height) {
            // Skip comments and whitespace
            while (pos < len) {
                if (allData[pos] == '#') {
                    while (pos < len && allData[pos] != '\n') pos++;
                } else if (allData[pos] == ' ' || allData[pos] == '\t' || allData[pos] == '\n' || allData[pos] == '\r') {
                    pos++;
                } else {
                    break;
                }
            }
            QString valStr;
            while (pos < len && allData[pos] != ' ' && allData[pos] != '\t' && allData[pos] != '\n' && allData[pos] != '\r') {
                valStr += allData[pos++];
            }
            if (valStr.isEmpty()) break;

            if (x < m_width) {
                int v = valStr.toInt();
                m_pixelData[y][x] = static_cast<unsigned char>(qBound(0, v, 255));
                ++x;
            } else {
                ++y;
                x = 0;
                if (y < m_height) {
                    int v = valStr.toInt();
                    m_pixelData[y][x] = static_cast<unsigned char>(qBound(0, v, 255));
                    ++x;
                }
            }
        }
    }

    file.close();

    // Create QImage for display
    m_originalImage = QImage(m_width, m_height, QImage::Format_Indexed8);
    
    // Set up grayscale color table
    QVector<QRgb> colorTable;
    for (int i = 0; i < 256; ++i) {
        colorTable.append(qRgb(i, i, i));
    }
    m_originalImage.setColorTable(colorTable);
    
    // Copy pixel data directly (no thresholding)
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            unsigned char val = m_pixelData[y][x];
            m_originalImage.setPixel(x, y, val);
        }
    }

    resetImage();
    
    // Center the image
    m_offset = QPoint((width() - m_width * m_zoom) / 2, 
                      (height() - m_height * m_zoom) / 2);

    emit imageModified();
    update();
    return true;
}

void ImageView::savePGM(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QByteArray header = "P5\n" + QByteArray::number(m_width) + " " + QByteArray::number(m_height) + "\n" + QByteArray::number(m_maxVal) + "\n";
    file.write(header);
    for (int y = 0; y < m_height; ++y) {
        file.write(reinterpret_cast<const char*>(m_pixelData[y].data()), m_width);
    }
    file.close();
}

void ImageView::resetImage()
{
    m_displayImage = m_originalImage.copy();
    m_zoom = 1.0;
    update();
}

void ImageView::setBrushSize(int size)
{
    m_brushSize = qMax(1, size);
}

int ImageView::brushSize() const
{
    return m_brushSize;
}

void ImageView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (m_displayImage.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "No image loaded\nDrag & drop a PGM file or use File > Open");
        return;
    }

    // Calculate scaled image rect
    int scaledWidth = static_cast<int>(m_width * m_zoom);
    int scaledHeight = static_cast<int>(m_height * m_zoom);
    QRect imageRect(m_offset.x(), m_offset.y(), scaledWidth, scaledHeight);

    // Draw image with nearest neighbor scaling for crisp binary pixels
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(imageRect, m_displayImage);

    // Draw border
    painter.setPen(QPen(Qt::white, 2));
    painter.drawRect(imageRect);

    // Draw brush preview circle when mouse is over image
    QPoint imgPos = screenToImage(m_currentMousePos);
    if (imgPos.x() >= 0) {
        painter.setPen(QPen(Qt::red, 1));
        painter.setBrush(Qt::NoBrush);
        int scaledRadius = static_cast<int>(m_brushSize * m_zoom / 2.0);
        QPoint screenCenter(
            m_offset.x() + static_cast<int>((imgPos.x() + 0.5) * m_zoom),
            m_offset.y() + static_cast<int>((imgPos.y() + 0.5) * m_zoom)
        );
        painter.drawEllipse(screenCenter, scaledRadius, scaledRadius);
    }

    // Draw info
    painter.setPen(Qt::lightGray);
    painter.drawText(10, 20, QString("Zoom: %1x").arg(m_zoom, 0, 'f', 2));
    painter.drawText(10, 40, QString("Brush: %1").arg(m_brushSize));
    painter.drawText(10, 60, QString("Size: %1x%2").arg(m_width).arg(m_height));
}

QPoint ImageView::screenToImage(const QPoint &screenPos)
{
    int scaledWidth = static_cast<int>(m_width * m_zoom);
    int scaledHeight = static_cast<int>(m_height * m_zoom);
    
    if (screenPos.x() < m_offset.x() || screenPos.x() >= m_offset.x() + scaledWidth ||
        screenPos.y() < m_offset.y() || screenPos.y() >= m_offset.y() + scaledHeight) {
        return QPoint(-1, -1);
    }

    int imgX = static_cast<int>((screenPos.x() - m_offset.x()) / m_zoom);
    int imgY = static_cast<int>((screenPos.y() - m_offset.y()) / m_zoom);
    
    imgX = qBound(0, imgX, m_width - 1);
    imgY = qBound(0, imgY, m_height - 1);
    
    return QPoint(imgX, imgY);
}

void ImageView::erasePixel(int x, int y)
{
    int radius = m_brushSize / 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
                if (dx*dx + dy*dy <= radius*radius) {
                    // Erase to white (255) instead of black (0)
                    m_pixelData[py][px] = 255;
                    m_displayImage.setPixel(px, py, 255);
                }
            }
        }
    }
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
        QPoint imgPos = screenToImage(event->pos());
        if (imgPos.x() >= 0) {
            m_isDrawing = true;
            erasePixel(imgPos.x(), imgPos.y());
            update();
        }
    } else if (event->button() == Qt::MiddleButton || 
               (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        // Start pan mode - store initial position
        m_lastPanPos = event->position().toPoint();
        m_isDrawing = false;
    }
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    // Always update current mouse position for brush preview
    m_currentMousePos = event->position().toPoint();
    
    if (m_isDrawing && event->buttons() & Qt::LeftButton) {
        QPoint imgPos = screenToImage(event->pos());
        if (imgPos.x() >= 0) {
            erasePixel(imgPos.x(), imgPos.y());
            update();
        }
    } else if (event->buttons() & Qt::MiddleButton ||
               (event->buttons() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        // Pan - use delta from last position
        QPoint currentPos = event->position().toPoint();
        m_offset += currentPos - m_lastPanPos;
        m_lastPanPos = currentPos;
        update();
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = false;
    }
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (m_displayImage.isNull()) return;

    double delta = event->angleDelta().y();
    double oldZoom = m_zoom;
    
    if (delta > 0) {
        m_zoom *= 1.1;
    } else {
        m_zoom /= 1.1;
    }
    
    m_zoom = qBound(0.1, m_zoom, 10.0);

    // Adjust offset to zoom towards cursor
    if (oldZoom != m_zoom) {
        QPoint cursorPos = event->position().toPoint();
        double factor = m_zoom / oldZoom;
        m_offset.setX(cursorPos.x() - (cursorPos.x() - m_offset.x()) * factor);
        m_offset.setY(cursorPos.y() - (cursorPos.y() - m_offset.y()) * factor);
    }
    
    update();
}

void ImageView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ImageView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filename = urls.first().toLocalFile();
            if (!filename.isEmpty()) {
                loadPGM(filename);
            }
        }
    }
}
