#include "../base/logging.h"
#include "queryresult.h"


QueryResult::QueryResult(MYSQL_RES *result, uint64_t rowCount, uint32_t fieldCount)
 : mFieldCount(fieldCount), mRowCount(rowCount)
{
    mResult = result;
	mCurrentRow = new Field[mFieldCount];
    assert(mCurrentRow); //断言判断的是上面的new是否成功,不成功就是0，即false，程序终止

    MYSQL_FIELD* fields = mysql_fetch_fields(mResult); //从结果集中取出所有的列
    for (uint32_t i = 0; i < mFieldCount; i++)
    {
        mFieldNames[i] = fields[i].name;  //保存列名
        m_vtFieldNames.push_back(fields[i].name);
        mCurrentRow[i].SetType(ConvertNativeType(fields[i].type)); //保存当前行的类型
    }
}

QueryResult::~QueryResult(void)
{
    EndQuery();
}

bool QueryResult::NextRow()
{
    MYSQL_ROW row;

    if (!mResult)
        return false;

    row = mysql_fetch_row(mResult); //每次fetch_row之后都拿到的是下一行，然后指针向后移动指向现在这一行
    if (!row)
    {
        EndQuery();
        return false;
    }

    unsigned long int *ulFieldLength; //列长度
    ulFieldLength = mysql_fetch_lengths(mResult);
    for (uint32_t i = 0; i < mFieldCount; i++)
    {
        if(row[i] == NULL)
        {
            mCurrentRow[i].m_bNULL = true;
            mCurrentRow[i].SetValue("", 0);
        }
        else
        {
            mCurrentRow[i].m_bNULL = false;
           mCurrentRow[i].SetValue(row[i], ulFieldLength[i]); //保存这一行的数据内容
        }

        mCurrentRow[i].SetName(mFieldNames[i]); //给这一行的每一格设置列名
    }

    return true;
}

void QueryResult::EndQuery()
{
    if (mCurrentRow)
    {
        delete[] mCurrentRow;
        mCurrentRow = 0;
    }

    if (mResult)
    {
		LOG_INFO << "QueryResult::EndQuery, mysql_free_result";
        mysql_free_result(mResult);
        mResult = 0;
    }
}

enum Field::DataTypes QueryResult::ConvertNativeType(enum_field_types mysqlType) const
{
    switch (mysqlType)
    {
        case FIELD_TYPE_TIMESTAMP:
        case FIELD_TYPE_DATE:
        case FIELD_TYPE_TIME:
        case FIELD_TYPE_DATETIME:
        case FIELD_TYPE_YEAR:
        case FIELD_TYPE_STRING:
        case FIELD_TYPE_VAR_STRING:
        case FIELD_TYPE_BLOB:
        case FIELD_TYPE_SET:
        case FIELD_TYPE_NULL:
            return Field::DB_TYPE_STRING;
        case FIELD_TYPE_TINY:

        case FIELD_TYPE_SHORT:
        case FIELD_TYPE_LONG:
        case FIELD_TYPE_INT24:
        case FIELD_TYPE_LONGLONG:
        case FIELD_TYPE_ENUM:
            return Field::DB_TYPE_INTEGER;
        case FIELD_TYPE_DECIMAL:
        case FIELD_TYPE_FLOAT:
        case FIELD_TYPE_DOUBLE:
            return Field::DB_TYPE_FLOAT;
        default:
            return Field::DB_TYPE_UNKNOWN;
    }
}
