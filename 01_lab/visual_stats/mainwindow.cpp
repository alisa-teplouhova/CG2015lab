#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "statstablemodel.h"
#include "statsdocument.h"
#include "insertrowdialog.h"
#include "aboutdialog.h"
#include <QTableView>
#include <QDebug>
#include <unordered_set>
#include <QMessageBox>
#include <QResizeEvent>
#include <QTimer>
#include <QSortFilterProxyModel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_tableModel = std::make_shared<StatsTableModel>();

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(m_tableModel.get());
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_ui->tableData->setModel(proxyModel);
    m_ui->tableData->setAlternatingRowColors(true);
    m_ui->tableData->setSortingEnabled(true);

    m_ui->newDocument->setShortcut(QKeySequence(QKeySequence::New));
    m_ui->openDocument->setShortcut(QKeySequence(QKeySequence::Open));
    m_ui->saveDocument->setShortcut(QKeySequence(QKeySequence::Save));
    m_ui->saveDocumentAs->setShortcut(QKeySequence(QKeySequence::SaveAs));
    m_ui->actionDeleteRow->setShortcut(QKeySequence(QKeySequence::Delete));
    m_ui->actionInsertRow->setShortcut(QKeySequence(tr("Insert")));

    m_document.reset(new StatsDocument(this, *m_tableModel));

    scene = new QGraphicsScene(this);
    m_ui->graphicsView->setScene(scene);

    drawPieChart();

    connect(m_tableModel.get(), SIGNAL(dataChanged(const QModelIndex, const QModelIndex)), this, SLOT(on_valueUpdated(const QModelIndex, const QModelIndex)));
    connect(m_tableModel.get(), SIGNAL(layoutChanged()), this, SLOT(on_tableUpdated()));
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::drawPieChart()
{
    int xOffset(0), yOffset(0);
    qreal size = m_ui->graphicsView->width() * 0.9;
    QRectF rect(xOffset, yOffset, size, size);

    auto model = m_tableModel->statsModel();
    qreal startAngle = 90;

    QColor color;
    qreal sum = model.valueSum();

    scene->clear();
    for (size_t i = 0; i < model.size(); i++)
    {
        qreal sweepLength = -1 * model.value(i) / sum * 360;
        color.setHsv((30 * i) % 360, 230, 240);
        addPieToChart(rect, startAngle, sweepLength, color);
        startAngle += sweepLength;
    }
}

void MainWindow::addPieToChart(QRectF & rect, qreal startAngle, qreal sweepLength, const QColor & color)
{
    QPainterPath path;
    path.moveTo(rect.center());
    path.arcTo(rect, startAngle, sweepLength);
    scene->addPath(path, QPen(Qt::white), QBrush(color));
}

void MainWindow::on_tableUpdated()
{
    drawPieChart();
}

void MainWindow::on_valueUpdated(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    drawPieChart();
}

bool MainWindow::checkUnsavedChanges()
{
    if (!m_tableModel->isSaved())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Warning"));
        msgBox.setText(tr("Do you want to save your changes?"));

        msgBox.addButton(QMessageBox::Save);
        msgBox.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::AcceptRole)
        {
            m_document->save();
            return true;
        }
        else if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::RejectRole)
        {
            return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (checkUnsavedChanges())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::on_newDocument_triggered()
{
    if (checkUnsavedChanges())
    {
        m_document->createNew();
    }
}

void MainWindow::on_saveDocument_triggered()
{
    m_document->save();
}

void MainWindow::on_openDocument_triggered()
{
    if (checkUnsavedChanges())
    {
        m_document->open();
    }
}

void MainWindow::on_saveDocumentAs_triggered()
{
    m_document->saveAs();
}

void MainWindow::on_showTable_triggered()
{
}

void MainWindow::on_showDiagram_triggered()
{
}

void MainWindow::on_actionAbout_triggered()
{
    std::unique_ptr<AboutDialog> dialog(new AboutDialog(this));
    dialog->exec();
}

void MainWindow::on_actionInsertRow_triggered()
{
    std::unique_ptr<InsertRowDialog> dialog(new InsertRowDialog(this));
    connect(dialog.get(), SIGNAL(rowReady(QString, int)), this, SLOT(onRowReady(QString, int)));
    dialog->exec();
}

void MainWindow::onRowReady(QString text, int value)
{
    auto model = m_tableModel->statsModel();
    model.append(text, value);
    m_tableModel->setStatsModel(model);
}

void MainWindow::on_actionDeleteRow_triggered()
{
    std::set<int> deletedRows = m_ui->tableData->selectedRows();
    if (deletedRows.size() == 0)
    {
        return;
    }

    auto statsModel = m_tableModel->statsModel();
    StatsKeyValueModel newModel;
    for (size_t i = 0, n = statsModel.size(); i < n; ++i)
    {
        if (deletedRows.count(i))
        {
            continue;
        }
        newModel.append(statsModel.key(i), statsModel.value(i));
    }
    m_tableModel->setStatsModel(newModel);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    QTimer::singleShot(0, this, SLOT(drawPieChart()));
}
