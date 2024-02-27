#pragma once

#include <assert.h>
#include <mysql/mysql.h>
#include <stdint.h>
#include <vector>
#include <map>

#include "field.h"

class  QueryResult
{
public:
    typedef std::map<uint32_t, std::string> FieldNames; //列名，集合存的是<列索引值，列名>

    QueryResult(MYSQL_RES* result, uint64_t rowCount, uint32_t fieldCount);
    virtual ~QueryResult();

    virtual bool NextRow();

    uint32_t GetField_idx(const std::string& name) const  //获取这一列的索引值
    {
        for(FieldNames::const_iterator iter = GetFieldNames().begin(); iter != GetFieldNames().end(); ++iter)
        {
            if(iter->second == name)
                return iter->first;
        }

        assert(false && "unknown field name"); //断言，直接断言为错，能运行到这肯定是错了，然后向stderr中打印"..."内容
        return uint32_t(-1);
    }

    Field *Fetch() const { return mCurrentRow; }

    const Field& operator[] (int index) const 
    { 
        return mCurrentRow[index];
    }

    const Field& operator[] (const std::string &name) const
    {
        return mCurrentRow[GetField_idx(name)];
    }

    uint32_t GetFieldCount() const { return mFieldCount; }
    uint64_t GetRowCount() const { return mRowCount; }
    FieldNames const& GetFieldNames() const {return mFieldNames; }

    vector<string> const& GetNames() const {return m_vtFieldNames;}

private:
    enum Field::DataTypes ConvertNativeType(enum_field_types mysqlType) const; //转换类型的那个函数
public:
    void EndQuery();

protected:
    Field *             mCurrentRow;
    uint32_t            mFieldCount;
    uint64_t            mRowCount;
    FieldNames          mFieldNames;
    std::vector<string> m_vtFieldNames;

	MYSQL_RES*          mResult;
};
