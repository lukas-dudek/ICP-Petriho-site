/** @brief Implementace panelu logu. @author Jan */
#include "log_panel.h"
#include "interpreter/log_entry.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

using pets::LogEntry;

LogPanel::LogPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    table_ = new QTableWidget(0, 3, this);
    table_->setHorizontalHeaderLabels({"Čas", "Typ", "Zpráva"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(table_);

    auto* btnLayout = new QHBoxLayout();
    auto* clearBtn = new QPushButton("Vymazat", this);
    auto* exportBtn = new QPushButton("Exportovat", this);
    btnLayout->addStretch();
    btnLayout->addWidget(clearBtn);
    btnLayout->addWidget(exportBtn);
    layout->addLayout(btnLayout);

    connect(clearBtn, &QPushButton::clicked, this, [this]() { clear(); });
    connect(exportBtn, &QPushButton::clicked, this, [this]() { exportToFile("log.txt"); });
}

void LogPanel::appendLog(const LogEntry& entry)
{
    int row = table_->rowCount();
    table_->insertRow(row);

    // Čas
    auto timet = std::chrono::system_clock::to_time_t(entry.timestamp);
    QString timeStr = QDateTime::fromSecsSinceEpoch(timet).toString("hh:mm:ss");
    table_->setItem(row, 0, new QTableWidgetItem(timeStr));

    // Typ
    table_->setItem(row, 1, new QTableWidgetItem(
        QString::fromStdString(entry.message)));

    // Detail
    table_->setItem(row, 2, new QTableWidgetItem(
        QString::fromStdString(entry.detail)));

    if (auto_scroll_) table_->scrollToBottom();
}

void LogPanel::clear()
{
    table_->setRowCount(0);
}

void LogPanel::exportToFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    for (int r = 0; r < table_->rowCount(); ++r) {
        out << table_->item(r, 0)->text() << "\t"
            << table_->item(r, 1)->text() << "\t"
            << table_->item(r, 2)->text() << "\n";
    }
}