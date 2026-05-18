/**
 * @file log_panel.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef LOG_PANEL_H
#define LOG_PANEL_H

#include <QWidget>
#include <QTableWidget>

namespace pets { struct LogEntry; }

using pets::LogEntry;

class LogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LogPanel(QWidget* parent = nullptr);

    void appendLog(const LogEntry& entry);
    void clear();
    void exportToFile(const QString& path);

private:
    QTableWidget* table_       = nullptr;
    bool          auto_scroll_ = true;
};

#endif