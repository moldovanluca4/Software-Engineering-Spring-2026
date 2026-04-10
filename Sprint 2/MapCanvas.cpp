#include "MapCanvasQt.h"

void MapCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);

    if (history.empty()) return;

    const FrameData& currentFrame = history.front();

    // Calculate the best cell size so the entire map fits inside the window perfectly
    int cellWidth = this->width() / currentFrame.Dimensions[0];
    int cellHeight = this->height() / currentFrame.Dimensions[1];
    
    // Pick the smaller of the two so it keeps its aspect ratio and doesn't stretch
    int cellSize = std::min(cellWidth, cellHeight);
    
    // Safety check just in case the map is ridiculously small or division fails
    if (cellSize < 1) cellSize = 1;

    
    for (size_t i = 0; i < history.size() - 1; ++i) {
       
        int alpha = 255 - (255 * (i + 1) / (max_steps + 1));
        if (alpha < 0) alpha = 0;

     
        QPoint r1 = getBugCenter(history[i], 'R', cellSize);
        QPoint r2 = getBugCenter(history[i+1], 'R', cellSize);
        if (!r1.isNull() && !r2.isNull()) {
            painter.setPen(QPen(QColor(255, 0, 0, alpha), 3));
            painter.drawLine(r1, r2);
        }

      
        QPoint b1 = getBugCenter(history[i], 'B', cellSize);
        QPoint b2 = getBugCenter(history[i+1], 'B', cellSize);
        if (!b1.isNull() && !b2.isNull()) {
            painter.setPen(QPen(QColor(51, 153, 255, alpha), 3));
            painter.drawLine(b1, b2);
        }
    }

   
    for (size_t r = 0; r < currentFrame.Map.size(); ++r) {
        int xOffset = (r % 2 != 0) ? cellSize / 2 : 0; 
        
        for (size_t c = 0; c < currentFrame.Map[r].size(); ++c) {
            char ch = currentFrame.Map[r][c];
            int x = c * cellSize + xOffset;
            int y = r * cellSize;
            
            if (ch == 'R' || ch == 'r') painter.setBrush(Qt::red);
            else if (ch == 'B' || ch == 'b') painter.setBrush(QColor(51, 153, 255));
            else if (ch == '#') painter.setBrush(Qt::gray);
            else if (isdigit(ch)) painter.setBrush(Qt::green);
            else continue; 

            painter.setPen(Qt::NoPen);
            painter.drawEllipse(x, y, cellSize - 2, cellSize - 2); 
        }
    }
}

QPoint MapCanvas::getBugCenter(const FrameData& frame, char targetBug, int cellSize) {
    for (size_t r = 0; r < frame.Map.size(); ++r) {
        int xOffset = (r % 2 != 0) ? cellSize / 2 : 0; 
        
        for (size_t c = 0; c < frame.Map[r].size(); ++c) {
          
            if (toupper(frame.Map[r][c]) == toupper(targetBug)) {
                int x = c * cellSize + xOffset;
                int y = r * cellSize;
                
                
                return QPoint(x + (cellSize / 2), y + (cellSize / 2));
            }
        }
    }
    return QPoint(); 
}