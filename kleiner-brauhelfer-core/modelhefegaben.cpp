#include "modelhefegaben.h"
#include "database_defs.h"
#include "brauhelfer.h"
#include <QDateTime>
#include <cmath>

ModelHefegaben::ModelHefegaben(Brauhelfer* bh, QSqlDatabase db) :
    SqlTableModel(bh, db),
    bh(bh)
{
    mVirtualField.append("ZugabeZeitpunkt");
    mVirtualField.append("Abfuellbereit");
    connect(bh->modelSud(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)),
            this, SLOT(onSudDataChanged(const QModelIndex&)));
}

QVariant ModelHefegaben::dataExt(const QModelIndex &index) const
{
    QString field = fieldName(index.column());
    if (field == "ZugabeZeitpunkt")
    {
        QVariant sudId = data(index.row(), "SudID");
        QDateTime braudatum = bh->modelSud()->getValueFromSameRow("ID", sudId, "Braudatum").toDateTime();
        if (braudatum.isValid())
            return braudatum.addDays(data(index.row(), "ZugegebenNach").toInt());
        return QDateTime();
    }
    if (field == "Abfuellbereit")
    {
        return data(index.row(), "Zugegeben").toBool();
    }
    return QVariant();
}

bool ModelHefegaben::setDataExt(const QModelIndex &index, const QVariant &value)
{
    QString field = fieldName(index.column());
    if (field == "ZugabeZeitpunkt")
    {
        QVariant sudId = data(index.row(), "SudID");
        QDateTime braudatum = bh->modelSud()->getValueFromSameRow("ID", sudId, "Braudatum").toDateTime();
        return QSqlTableModel::setData(index.siblingAtColumn(fieldIndex("ZugegebenNach")), braudatum.daysTo(value.toDateTime()));
    }
    return false;
}

void ModelHefegaben::onSudDataChanged(const QModelIndex &idx)
{
    QString field = bh->modelSud()->fieldName(idx.column());
    if (field == "BierWurdeGebraut")
    {
        int sudId = bh->modelSud()->data(idx.row(), "ID").toInt();
        int colSudId = fieldIndex("SudID");
        int colZugegeben = fieldIndex("Zugegeben");
        int colZugegebenNach = fieldIndex("ZugegebenNach");
        if (idx.data().toBool())
        {
            for (int row = 0; row < rowCount(); ++row)
            {
                if (data(index(row, colSudId)).toInt() == sudId &&
                    data(index(row, colZugegebenNach)).toInt() == 0)
                    setData(index(row, colZugegeben), true);
            }
        }
        else
        {
            for (int row = 0; row < rowCount(); ++row)
            {
                if (data(index(row, colSudId)).toInt() == sudId)
                    setData(index(row, colZugegeben), false);
            }
        }
    }
}

void ModelHefegaben::defaultValues(QVariantMap &values) const
{
    if (values.contains("SudID"))
    {
        QVariant sudId = values.value("SudID");
        if (bh->modelSud()->getValueFromSameRow("ID", sudId, "BierWurdeGebraut").toBool())
        {
            QDateTime braudatum = bh->modelSud()->getValueFromSameRow("ID", sudId, "Braudatum").toDateTime();
            if (!values.contains("ZugegebenNach"))
                values.insert("ZugegebenNach", braudatum.daysTo(QDateTime::currentDateTime()));
        }
    }
    if (!values.contains("Name"))
        values.insert("Name", bh->modelHefe()->data(0, "Beschreibung"));
    if (!values.contains("Menge"))
        values.insert("Menge", 1);
    if (!values.contains("Zugegeben"))
        values.insert("Zugegeben", 0);
    if (!values.contains("ZugegebenNach"))
        values.insert("ZugegebenNach", 0);  
}