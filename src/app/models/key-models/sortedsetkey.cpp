#include "sortedsetkey.h"
#include "modules/redisclient/command.h"
#include "modules/redisclient/commandexecutor.h"

SortedSetKeyModel::SortedSetKeyModel(QSharedPointer<RedisClient::Connection> connection, QString fullPath, int dbIndex, int ttl)
    : KeyModel(connection, fullPath, dbIndex, ttl)
{
    loadRowCount();
}

QString SortedSetKeyModel::getType()
{
    return "zset";
}

QStringList SortedSetKeyModel::getColumnNames()
{
    return QStringList() << "row" << "value" << "score";
}

QHash<int, QByteArray> SortedSetKeyModel::getRoles()
{
    QHash<int, QByteArray> roles;
    roles[Roles::RowNumber] = "row";
    roles[Roles::Value] = "value";
    roles[Roles::Score] = "score";
    return roles;
}

QVariant SortedSetKeyModel::getData(int rowIndex, int dataRole)
{
    if (!isRowLoaded(rowIndex))
        return QVariant();

    QPair<QByteArray, double> row = m_rowsCache[rowIndex];

    if (dataRole == Roles::Value)
        return row.first;
    else if (dataRole ==Roles::Score)
        return QString::number(row.second);
    else if (dataRole == Roles::RowNumber)
        return QString::number(rowIndex+1);

    return QVariant();
}

void SortedSetKeyModel::updateRow(int rowIndex, const QVariantMap &row)
{
    if (!isRowLoaded(rowIndex) || !isRowValid(row))
        throw Exception("Invalid row");

    QPair<QByteArray, double> cachedRow = m_rowsCache[rowIndex];

    bool valueChanged = cachedRow.first != row["value"].toString();
    bool scoreChanged = cachedRow.second != row["score"].toDouble();

    QPair<QByteArray, double> newRow(
                    (valueChanged) ? row["value"].toByteArray() : cachedRow.first,
                    (scoreChanged) ? row["score"].toDouble() : cachedRow.second
                );

    // TODO (uglide): Update only score if value not changed

    deleteSortedSetRow(cachedRow.first);
    addSortedSetRow(newRow.first, newRow.second);
    m_rowsCache.replace(rowIndex, newRow);
}

void SortedSetKeyModel::addRow(const QVariantMap &row)
{
    if (!isRowValid(row))
        throw Exception("Invalid row");

    addSortedSetRow(row["value"].toString(), row["score"].toDouble());
}

unsigned long SortedSetKeyModel::rowsCount()
{
    return m_rowCount;
}

void SortedSetKeyModel::loadRows(unsigned long rowStart, unsigned long count, std::function<void ()> callback)
{
    if (isPartialLoadingSupported()) {
        //TBD
    } else {
        QStringList rows = getRowsRange("ZRANGE WITHSCORES", rowStart, count).toStringList();       

        for (QStringList::iterator item = rows.begin();
             item != rows.end(); ++item) {

            QPair<QByteArray, double> value;
            value.first = item->toUtf8();
            ++item;

            if (item == rows.end())
                throw Exception("Partial data loaded from server");

            value.second = item->toDouble();
            m_rowsCache.push_back(value);
        }
    }

    callback();
}

void SortedSetKeyModel::clearRowCache()
{
    m_rowsCache.clear();
}

void SortedSetKeyModel::removeRow(int i)
{
    if (!isRowLoaded(i))
        return;

    QByteArray value = m_rowsCache.value(i).first;

    using namespace RedisClient;

    Command deleteValues(QStringList() << "ZREM" << m_keyFullPath << value, m_dbIndex);
    Response result = CommandExecutor::execute(m_connection, deleteValues);

    m_rowCount--;
    m_rowsCache.removeAt(i);
    Q_UNUSED(result);

    setRemovedIfEmpty();
}

bool SortedSetKeyModel::isRowLoaded(int rowIndex)
{
    return 0 <= rowIndex && rowIndex < m_rowsCache.size();
}

bool SortedSetKeyModel::isMultiRow() const
{
    return true;
}

void SortedSetKeyModel::loadRowCount()
{
    m_rowCount = getRowCount("ZCARD");
}

void SortedSetKeyModel::addSortedSetRow(const QString &value, double score)
{
    using namespace RedisClient;
    Command addCmd(QStringList() << "ZADD" << m_keyFullPath
                   << QString::number(score) << value, m_dbIndex);
    CommandExecutor::execute(m_connection, addCmd);
}

void SortedSetKeyModel::deleteSortedSetRow(const QString &value)
{
    using namespace RedisClient;
    Command addCmd(QStringList() << "ZREM" << m_keyFullPath << value, m_dbIndex);
    CommandExecutor::execute(m_connection, addCmd);
}
