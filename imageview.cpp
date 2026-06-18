#include "imageview.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QBrush>
#include <QPen>
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

    QTextStream in(&file);
    QString magic;
    in >> magic;

    if (magic != "P2" && magic != "P5") {
        file.close();
        return false;
    }

    // Skip comments and read dimensions
    QString line;
    while (in.device()->pos() < file.size()) {
        line = in.readLine().trimmed();
        if (!line.startsWith('#') && !line.isEmpty()) {
            QStringList parts = line.split(QChar(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                m_width = parts[0].toInt();
                m_height = parts[1].toInt();
                break;
            } else if (parts.size() == 1) {
                m_width = parts[0].toInt();
                // Read height from next non-comment line
                while (in.device()->pos() < file.size()) {
                    line = in.readLine().trimmed();
                    if (!line.startsWith('#') && !line.isEmpty()) {
                        m_height = line.toInt();
                        break;
                    }
                }
                break;
            }
        }
    }

    // Read max value
    while (in.device()->pos() < file.size()) {
        line = in.readLine().trimmed();
        if (!line.startsWith('#') && !line.isEmpty()) {
            m_maxVal = line.toInt();
            break;
        }
    }

    if (m_width <= 0 || m_height <= 0 || m_maxVal <= 0) {
        file.close();
        return false;
    }

    // Initialize pixel data
    m_pixelData.resize(m_height, std::vector<unsigned char>(m_width));

    // Read pixel data - handle P5 binary format properly
    if (magic == "P5") {
        // Binary format - read raw bytes after header
        qint64 pos = in.device()->pos();
        file.seek(pos);
        QByteArray data = file.readAll();
        int idx = 0;
        for (int y = 0; y < m_height && idx < data.size(); ++y) {
            for (int x = 0; x < m_width && idx < data.size(); ++x) {
                m_pixelData[y][x] = static_cast<unsigned char>(data[idx++]);
            }
        }
    } else {
        // ASCII format (P2)
        int x = 0, y = 0;
        while (!in.atEnd() && y < m_height) {
            line = in.readLine().trimmed();
            if (line.startsWith('#') || line.isEmpty()) continue;
            
            QStringList values = line.split(QChar(' '), Qt::SkipEmptyParts);
            for (const QString &val : values) {
                if (x >= m_width) {
                    x = 0;
                    ++y;
                }
                if (y < m_height && x < m_width) {
                    int v = val.toInt();
                    m_pixelData[y][x] = static_cast<unsigned char>(qBound(0, v, 255));
                    ++x;
                }
            }
        }
    }

    file.close();

    // Create QImage for display - use Format_Indexed8 for binary (black/white only)
    m_originalImage = QImage(m_width, m_height, QImage::Format_Indexed8);
    
    // Set up color table: index 0 = black, index 255 = white
    QVector<QRgb> colorTable;
    for (int i = 0; i < 256; ++i) {
        if (i == 0) {
            colorTable.append(qRgb(0, 0, 0));      // Black
        } else if (i == 255) {
            colorTable.append(qRgb(255, 255, 255)); // White
        } else {
            // For intermediate values, threshold to black or white
            colorTable.append(i > 127 ? qRgb(255, 255, 255) : qRgb(0, 0, 0));
        }
    }
    m_originalImage.setColorTable(colorTable);
    
    // Convert pixel data to binary (0 or 255 only)
    int threshold = m_maxVal / 2;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            unsigned char val = m_pixelData[y][x];
            // Threshold to binary: <= threshold becomes 0 (black), > threshold becomes 255 (white)
            m_pixelData[y][x] = (val <= threshold) ? 0 : 255;
            m_originalImage.setPixel(x, y, m_pixelData[y][x]);
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

    QTextStream out(&file);
    out << "P5\n";
    out << m_width << " " << m_height << "\n";
    out << m_maxVal << "\n";

    QByteArray data;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            data.append(static_cast<char>(m_pixelData[y][x]));
        }
    }
    file.write(data);
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
    if (imgPos.x() >= 0 && !m_isDrawing) {
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
