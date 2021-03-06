#include "tabsudauswahl.h"
#include "ui_tabsudauswahl.h"
#include <QMessageBox>
#include <QMenu>
#include <QFileInfo>
#include <QFileDialog>
#include <QDesktopServices>
#include "brauhelfer.h"
#include "settings.h"
#include "model/proxymodelsudcolored.h"
#include "model/checkboxdelegate.h"
#include "model/comboboxdelegate.h"
#include "model/datedelegate.h"
#include "model/doublespinboxdelegate.h"
#include "model/ebcdelegate.h"
#include "model/linklabeldelegate.h"
#include "model/ratingdelegate.h"
#include "model/spinboxdelegate.h"

extern Brauhelfer* bh;
extern Settings* gSettings;

TabSudAuswahl::TabSudAuswahl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabSudAuswahl),
    mTempCssFile(QDir::tempPath() + "/" + QCoreApplication::applicationName() + QLatin1String(".XXXXXX.css"))
{
    ui->setupUi(this);

    ui->tbTemplate->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->tbTemplate->setTabStopDistance(2 * QFontMetrics(ui->tbTemplate->font()).width(' '));
    ui->btnSaveTemplate->setPalette(gSettings->paletteErrorButton);
    ui->treeViewTemplateTags->setColumnWidth(0, 150);
    ui->treeViewTemplateTags->setColumnWidth(1, 150);

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);

    mHtmlHightLighter = new HtmlHighLighter(ui->tbTemplate->document());
    ui->webview->setTemplateFile(gSettings->dataDir() + "sudinfo.html");

    SqlTableModel *model = bh->modelSud();
    model->setHeaderData(model->fieldIndex("Sudnummer"), Qt::Horizontal, tr("#"));
    model->setHeaderData(model->fieldIndex("Sudname"), Qt::Horizontal, tr("Sudname"));
    model->setHeaderData(model->fieldIndex("Braudatum"), Qt::Horizontal, tr("Braudatum"));
    model->setHeaderData(model->fieldIndex("Erstellt"), Qt::Horizontal, tr("Erstellt"));
    model->setHeaderData(model->fieldIndex("Gespeichert"), Qt::Horizontal, tr("Gespeichert"));
    model->setHeaderData(model->fieldIndex("BewertungMax"), Qt::Horizontal, tr("Bewertung"));
    model->setHeaderData(model->fieldIndex("erg_AbgefuellteBiermenge"), Qt::Horizontal, tr("Menge [l]"));
    model->setHeaderData(model->fieldIndex("erg_Sudhausausbeute"), Qt::Horizontal, tr("SHA [%]"));
    model->setHeaderData(model->fieldIndex("SWIst"), Qt::Horizontal, tr("SW [°P]"));
    model->setHeaderData(model->fieldIndex("erg_S_Gesammt"), Qt::Horizontal, tr("Schüttung [kg]"));
    model->setHeaderData(model->fieldIndex("erg_Preis"), Qt::Horizontal, tr("Kosten [%1/l]").arg(QLocale().currencySymbol()));
    model->setHeaderData(model->fieldIndex("erg_Alkohol"), Qt::Horizontal, tr("Alk. [%]"));
    model->setHeaderData(model->fieldIndex("sEVG"), Qt::Horizontal, tr("sEVG [%]"));
    model->setHeaderData(model->fieldIndex("tEVG"), Qt::Horizontal, tr("tEVG [%]"));
    model->setHeaderData(model->fieldIndex("erg_EffektiveAusbeute"), Qt::Horizontal, tr("Eff. SHA [%]"));
    model->setHeaderData(model->fieldIndex("Verdampfungsziffer"), Qt::Horizontal, tr("Verdampfungsziffer [%]"));
    model->setHeaderData(model->fieldIndex("AusbeuteIgnorieren"), Qt::Horizontal, tr("Für Durchschnitt Ignorieren"));

    int col;
    QTableView *table = ui->tableSudauswahl;
    QHeaderView *header = table->horizontalHeader();
    ProxyModelSudColored *proxyModel = new ProxyModelSudColored(this);
    proxyModel->setSourceModel(model);
    table->setModel(proxyModel);
    for (int col = 0; col < model->columnCount(); ++col)
        table->setColumnHidden(col, true);

    col = model->fieldIndex("Sudname");
    table->setColumnHidden(col, false);
    header->setSectionResizeMode(col, QHeaderView::Stretch);
    header->moveSection(header->visualIndex(col), 0);

    col = model->fieldIndex("Sudnummer");
    table->setColumnHidden(col, false);
    table->setItemDelegateForColumn(col, new SpinBoxDelegate(table));
    header->resizeSection(col, 50);
    header->moveSection(header->visualIndex(col), 1);

    col = model->fieldIndex("Braudatum");
    table->setColumnHidden(col, false);
    table->setItemDelegateForColumn(col, new DateDelegate(false, true, table));
    header->resizeSection(col, 100);
    header->moveSection(header->visualIndex(col), 2);

    col = model->fieldIndex("Erstellt");
    table->setColumnHidden(col, false);
    table->setItemDelegateForColumn(col, new DateDelegate(false, true, table));
    header->resizeSection(col, 150);
    header->moveSection(header->visualIndex(col), 3);

    col = model->fieldIndex("Gespeichert");
    table->setColumnHidden(col, false);
    table->setItemDelegateForColumn(col, new DateDelegate(false, true, table));
    header->resizeSection(col, 150);
    header->moveSection(header->visualIndex(col), 4);

    col = model->fieldIndex("BewertungMax");
    table->setColumnHidden(col, false);
    table->setItemDelegateForColumn(col, new RatingDelegate(table));
    header->moveSection(header->visualIndex(col), 5);

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(on_tableSudauswahl_customContextMenuRequested(const QPoint&)));

    gSettings->beginGroup("TabSudAuswahl");

    mDefaultTableState = header->saveState();
    header->restoreState(gSettings->value("tableSudAuswahlState").toByteArray());

    mDefaultSplitterState = ui->splitter->saveState();
    ui->splitter->restoreState(gSettings->value("splitterState").toByteArray());

    ProxyModelSud::FilterStatus filterStatus = static_cast<ProxyModelSud::FilterStatus>(gSettings->value("filterStatus", ProxyModelSud::FilterStatus::Alle).toInt());
    proxyModel->setFilterStatus(filterStatus);
    ui->rbAlle->setChecked(filterStatus == ProxyModelSud::Alle);
    ui->rbNichtGebraut->setChecked(filterStatus == ProxyModelSud::NichtGebraut);
    ui->rbNichtAbgefuellt->setChecked(filterStatus == ProxyModelSud::GebrautNichtAbgefuellt);
    ui->rbNichtVerbraucht->setChecked(filterStatus == ProxyModelSud::NichtVerbraucht);
    ui->rbAbgefuellt->setChecked(filterStatus == ProxyModelSud::Abgefuellt);
    ui->rbVerbraucht->setChecked(filterStatus == ProxyModelSud::Verbraucht);
    proxyModel->setFilterStatus(filterStatus);

    ui->cbMerkliste->setChecked(gSettings->value("filterMerkliste", false).toBool());

    ui->tbDatumVon->setDate(gSettings->value("ZeitraumVon").toDate());
    ui->tbDatumBis->setDate(gSettings->value("ZeitraumBis").toDate());
    ui->cbDatumAlle->setChecked(gSettings->value("ZeitraumAlle", true).toBool());

    gSettings->endGroup();

    connect(bh, SIGNAL(modified()), this, SLOT(databaseModified()));
    connect(proxyModel, SIGNAL(layoutChanged()), this, SLOT(filterChanged()));
    connect(ui->tableSudauswahl->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(selectionChanged()));

    ui->tableSudauswahl->selectRow(0);
    filterChanged();
    on_cbEditMode_clicked(ui->cbEditMode->isChecked());
}

TabSudAuswahl::~TabSudAuswahl()
{
    delete ui;
}

void TabSudAuswahl::saveSettings()
{
    gSettings->beginGroup("TabSudAuswahl");
    gSettings->setValue("tableSudAuswahlState", ui->tableSudauswahl->horizontalHeader()->saveState());
    gSettings->setValue("filterStatus", static_cast<ProxyModelSud*>(ui->tableSudauswahl->model())->filterStatus());
    gSettings->setValue("filterMerkliste", ui->cbMerkliste->isChecked());
    gSettings->setValue("ZeitraumVon", ui->tbDatumVon->date());
    gSettings->setValue("ZeitraumBis", ui->tbDatumBis->date());
    gSettings->setValue("ZeitraumAlle", ui->cbDatumAlle->isChecked());
    gSettings->setValue("splitterState", ui->splitter->saveState());
    gSettings->endGroup();
}

void TabSudAuswahl::restoreView()
{
    ui->tableSudauswahl->horizontalHeader()->restoreState(mDefaultTableState);
    ui->splitter->restoreState(mDefaultSplitterState);
}

QAbstractItemModel* TabSudAuswahl::model() const
{
    return ui->tableSudauswahl->model();
}

void TabSudAuswahl::databaseModified()
{
    updateTemplateTags();
    erstelleSudInfo();
}

void TabSudAuswahl::filterChanged()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    ui->lblNumSude->setText(QString::number(model->rowCount()));
}

void TabSudAuswahl::selectionChanged()
{
    bool selected = ui->tableSudauswahl->selectionModel()->selectedRows().count() > 0;
    updateTemplateTags();
    erstelleSudInfo();
    ui->btnMerken->setEnabled(selected);
    ui->btnVergessen->setEnabled(selected);
    ui->btnKopieren->setEnabled(selected);
    ui->btnLoeschen->setEnabled(selected);
    ui->btnExportieren->setEnabled(selected);
    ui->btnLaden->setEnabled(selected);
}

void TabSudAuswahl::on_tableSudauswahl_doubleClicked(const QModelIndex &index)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int sudId = model->data(index.row(), "ID").toInt();
    clicked(sudId);
}

void TabSudAuswahl::spalteAnzeigen(bool checked)
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action)
        ui->tableSudauswahl->setColumnHidden(action->data().toInt(), !checked);
}

void TabSudAuswahl::on_tableSudauswahl_customContextMenuRequested(const QPoint &pos)
{
    int col;
    QMenu menu(this);
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());

    QAction action1(&menu);
    QAction action2(&menu);
    if (!ui->tableSudauswahl->horizontalHeader()->rect().contains(pos))
    {
        QModelIndex index = ui->tableSudauswahl->indexAt(pos);
        if (model->data(index.row(), "MerklistenID").toBool())
        {
            action1.setText(tr("Sud vergessen"));
            connect(&action1, SIGNAL(triggered()), this, SLOT(on_btnVergessen_clicked()));
        }
        else
        {
            action1.setText(tr("Sud merken"));
            connect(&action1, SIGNAL(triggered()), this, SLOT(on_btnMerken_clicked()));
        }
        menu.addAction(&action1);

        if (model->data(index.row(), "BierWurdeAbgefuellt").toBool())
        {
            if (model->data(index.row(), "BierWurdeVerbraucht").toBool())
            {
                action2.setText(tr("Sud nicht verbraucht"));
                connect(&action2, SIGNAL(triggered()), this, SLOT(onNichtVerbraucht_clicked()));
            }
            else
            {
                action2.setText(tr("Sud verbraucht"));
                connect(&action2, SIGNAL(triggered()), this, SLOT(onVerbraucht_clicked()));
            }
            menu.addAction(&action2);
        }

        menu.addSeparator();
    }

    QAction action3(tr("Sudnummer"), &menu);
    col = model->fieldIndex("Sudnummer");
    action3.setCheckable(true);
    action3.setChecked(!ui->tableSudauswahl->isColumnHidden(col));
    action3.setData(col);
    connect(&action3, SIGNAL(triggered(bool)), this, SLOT(spalteAnzeigen(bool)));
    menu.addAction(&action3);

    QAction action4(tr("Braudatum"), &menu);
    col = model->fieldIndex("Braudatum");
    action4.setCheckable(true);
    action4.setChecked(!ui->tableSudauswahl->isColumnHidden(col));
    action4.setData(col);
    connect(&action4, SIGNAL(triggered(bool)), this, SLOT(spalteAnzeigen(bool)));
    menu.addAction(&action4);

    QAction action5(tr("Erstellt"), &menu);
    col = model->fieldIndex("Erstellt");
    action5.setCheckable(true);
    action5.setChecked(!ui->tableSudauswahl->isColumnHidden(col));
    action5.setData(col);
    connect(&action5, SIGNAL(triggered(bool)), this, SLOT(spalteAnzeigen(bool)));
    menu.addAction(&action5);

    QAction action6(tr("Gespeichert"), &menu);
    col = model->fieldIndex("Gespeichert");
    action6.setCheckable(true);
    action6.setChecked(!ui->tableSudauswahl->isColumnHidden(col));
    action6.setData(col);
    connect(&action6, SIGNAL(triggered(bool)), this, SLOT(spalteAnzeigen(bool)));
    menu.addAction(&action6);

    QAction action7(tr("Bewertung"), &menu);
    col = model->fieldIndex("BewertungMax");
    action7.setCheckable(true);
    action7.setChecked(!ui->tableSudauswahl->isColumnHidden(col));
    action7.setData(col);
    connect(&action7, SIGNAL(triggered(bool)), this, SLOT(spalteAnzeigen(bool)));
    menu.addAction(&action7);

    menu.exec(ui->tableSudauswahl->viewport()->mapToGlobal(pos));
}

void TabSudAuswahl::on_rbAlle_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::Alle);
}

void TabSudAuswahl::on_rbNichtGebraut_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::NichtGebraut);
}

void TabSudAuswahl::on_rbNichtAbgefuellt_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::GebrautNichtAbgefuellt);
}

void TabSudAuswahl::on_rbNichtVerbraucht_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::NichtVerbraucht);
}

void TabSudAuswahl::on_rbAbgefuellt_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::Abgefuellt);
}

void TabSudAuswahl::on_rbVerbraucht_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterStatus(ProxyModelSud::Verbraucht);
}

void TabSudAuswahl::on_cbMerkliste_stateChanged(int state)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterMerkliste(state);
}

void TabSudAuswahl::on_tbFilter_textChanged(const QString &pattern)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterText(pattern);
}

void TabSudAuswahl::on_tbDatumVon_dateChanged(const QDate &date)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterMinimumDate(QDateTime(date.addDays(-1)));
    ui->tbDatumBis->setMinimumDate(date);
}

void TabSudAuswahl::on_tbDatumBis_dateChanged(const QDate &date)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    model->setFilterMaximumDate(QDateTime(date.addDays(1)));
    ui->tbDatumVon->setMaximumDate(date);
}

void TabSudAuswahl::on_cbDatumAlle_stateChanged(int state)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    if (state)
        model->setFilterDateColumn(-1);
    else
        model->setFilterDateColumn(bh->modelSud()->fieldIndex("Braudatum"));
    ui->tbDatumVon->setEnabled(!state);
    ui->tbDatumBis->setEnabled(!state);
}

void TabSudAuswahl::on_btnMerken_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int col = model->fieldIndex("MerklistenID");
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        QModelIndex indexMerkliste = index.siblingAtColumn(col);
        if (!model->data(indexMerkliste).toBool())
            model->setData(indexMerkliste, true);
    }
    ui->tableSudauswahl->setFocus();
}

void TabSudAuswahl::on_btnVergessen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int col = model->fieldIndex("MerklistenID");
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        QModelIndex indexMerkliste = index.siblingAtColumn(col);
        if (model->data(indexMerkliste).toBool())
            model->setData(indexMerkliste, false);
    }
    ui->tableSudauswahl->setFocus();
}

void TabSudAuswahl::on_btnAlleVergessen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int col = model->fieldIndex("MerklistenID");
    for (int row = 0; row < model->rowCount(); ++row)
    {
        QModelIndex index = model->index(row, col);
        if (model->data(index).toBool())
            model->setData(index, false);
    }
    ui->tableSudauswahl->setFocus();
}

void TabSudAuswahl::onVerbraucht_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int col = model->fieldIndex("BierWurdeVerbraucht");
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        QModelIndex indexVerbraucht = index.siblingAtColumn(col);
        if (!model->data(indexVerbraucht).toBool())
            model->setData(indexVerbraucht, true);
    }
    ui->tableSudauswahl->setFocus();
}

void TabSudAuswahl::onNichtVerbraucht_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    int col = model->fieldIndex("BierWurdeVerbraucht");
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        QModelIndex indexVerbraucht = index.siblingAtColumn(col);
        if (model->data(indexVerbraucht).toBool())
            model->setData(indexVerbraucht, false);
    }
    ui->tableSudauswahl->setFocus();
}

void TabSudAuswahl::on_btnAnlegen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    if (!ui->rbNichtGebraut->isChecked() && !ui->rbAlle->isChecked())
    {
        ui->rbAlle->setChecked(true);
        model->setFilterStatus(ProxyModelSud::Alle);
    }
    ui->cbMerkliste->setChecked(false);
    ui->cbDatumAlle->setChecked(true);
    ui->tbFilter->clear();

    QVariantMap values({{"Sudname", tr("Neuer Sud")}});
    int row = model->append(values);
    if (row >= 0)
    {
        filterChanged();
        ui->tableSudauswahl->setCurrentIndex(model->index(row, model->fieldIndex("Sudname")));
        ui->tableSudauswahl->scrollTo(ui->tableSudauswahl->currentIndex());
        ui->tableSudauswahl->edit(ui->tableSudauswahl->currentIndex());
    }
}

void TabSudAuswahl::on_btnKopieren_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    if (!ui->rbNichtGebraut->isChecked() && !ui->rbAlle->isChecked())
    {
        ui->rbAlle->setChecked(true);
        model->setFilterStatus(ProxyModelSud::Alle);
    }
    ui->cbMerkliste->setChecked(false);
    ui->cbDatumAlle->setChecked(true);
    ui->tbFilter->clear();

    int row = -1;
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        int sudId = model->data(index.row(), "ID").toInt();
        QString name = model->data(index.row(), "Sudname").toString() + " " + tr("Kopie");
        row = bh->sudKopieren(sudId, name);
    }
    if (row >= 0)
    {
        filterChanged();
        ui->tableSudauswahl->setCurrentIndex(model->index(row, model->fieldIndex("Sudname")));
        ui->tableSudauswahl->scrollTo(ui->tableSudauswahl->currentIndex());
        ui->tableSudauswahl->edit(ui->tableSudauswahl->currentIndex());
    }
}

void TabSudAuswahl::on_btnLoeschen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    QList<int> sudIds;
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
        sudIds.append(model->data(index.row(), "ID").toInt());
    for (int sudId : sudIds)
    {
        int row = model->getRowWithValue("ID", sudId);
        QString name = model->data(row, "Sudname").toString();
        int ret = QMessageBox::question(this, tr("Sud löschen?"),
                                        tr("Soll der Sud \"%1\" gelöscht werden?").arg(name));
        if (ret == QMessageBox::Yes)
        {
            if (model->removeRow(row))
                filterChanged();
        }
    }
}

void TabSudAuswahl::on_btnImportieren_clicked()
{
    // TODO: implement function
    QMessageBox::warning(this, tr("Sud importieren"), tr("Diese Funktion ist noch nicht implementiert."));
    filterChanged();
}

void TabSudAuswahl::on_btnExportieren_clicked()
{
    // TODO: implement function
    for (const QModelIndex &index : ui->tableSudauswahl->selectionModel()->selectedRows())
    {
        QMessageBox::warning(this, tr("Sud exportieren"), tr("Diese Funktion ist noch nicht implementiert."));
    }
}

void TabSudAuswahl::on_btnLaden_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableSudauswahl->model());
    QModelIndexList selection = ui->tableSudauswahl->selectionModel()->selectedRows();
    if (selection.count() > 0)
    {
        int sudId = model->data(selection[0].row(), "ID").toInt();
        clicked(sudId);
    }
}

void TabSudAuswahl::on_btnToPdf_clicked()
{
    QModelIndexList selection = ui->tableSudauswahl->selectionModel()->selectedRows();
    if (selection.count() == 0)
        return;

    gSettings->beginGroup("General");

    QString path = gSettings->value("exportPath", QDir::homePath()).toString();

    QString fileName;
    if (selection.count() == 1)
        fileName = static_cast<ProxyModel*>(ui->tableSudauswahl->model())->data(selection[0].row(), "Sudname").toString() + "_" + tr("Rohstoffe");
    else
        fileName = tr("Rohstoffe");

    QString filePath = QFileDialog::getSaveFileName(this, tr("PDF speichern unter"),
                                     path + "/" + fileName +  ".pdf", "PDF (*.pdf)");
    if (!filePath.isEmpty())
    {
        gSettings->setValue("exportPath", QFileInfo(filePath).absolutePath());
        ui->webview->printToPdf(filePath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }

    gSettings->endGroup();
}

void TabSudAuswahl::checkSaveTemplate()
{
    if (ui->btnSaveTemplate->isVisible())
    {
        int ret = QMessageBox::question(this, tr("Änderungen speichern?"),
                                        tr("Sollen die Änderungen gespeichert werden?"));
        if (ret == QMessageBox::Yes)
            on_btnSaveTemplate_clicked();
    }
}

void TabSudAuswahl::on_cbEditMode_clicked(bool checked)
{
    checkSaveTemplate();

    ui->tbTemplate->setVisible(checked);
    ui->treeViewTemplateTags->setVisible(checked);
    ui->btnRestoreTemplate->setVisible(checked);
    ui->cbTemplateAuswahl->setVisible(checked);
    ui->btnSaveTemplate->setVisible(false);
    ui->splitter_1->setHandleWidth(checked ? 5 : 0);

    if (checked)
    {
        QFile file(gSettings->dataDir() + ui->cbTemplateAuswahl->currentText());
        ui->btnSaveTemplate->setProperty("file", file.fileName());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            ui->tbTemplate->setPlainText(file.readAll());
            file.close();
        }
    }

    erstelleSudInfo();
}

void TabSudAuswahl::on_cbTemplateAuswahl_currentIndexChanged(int)
{
    on_cbEditMode_clicked(ui->cbEditMode->isChecked());
}

void TabSudAuswahl::on_tbTemplate_textChanged()
{
    if (ui->tbTemplate->hasFocus())
    {
        erstelleSudInfo();
        ui->btnSaveTemplate->setVisible(true);
    }
}

void TabSudAuswahl::on_btnSaveTemplate_clicked()
{
    QFile file(ui->btnSaveTemplate->property("file").toString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(ui->tbTemplate->toPlainText().toUtf8());
    file.close();
    ui->btnSaveTemplate->setVisible(false);
}

void TabSudAuswahl::on_btnRestoreTemplate_clicked()
{
    int ret = QMessageBox::question(this, tr("Template wiederherstellen?"),
                                    tr("Soll das Standardtemplate wiederhergestellt werden?"));
    if (ret == QMessageBox::Yes)
    {
        QFile file(gSettings->dataDir() + ui->cbTemplateAuswahl->currentText());
        QFile file2(":/data/" + ui->cbTemplateAuswahl->currentText());
        file.remove();
        if (file2.copy(file.fileName()))
            file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        on_cbEditMode_clicked(ui->cbEditMode->isChecked());
    }
}
