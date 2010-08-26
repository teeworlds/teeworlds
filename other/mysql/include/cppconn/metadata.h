/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _SQL_METADATA_H_
#define _SQL_METADATA_H_

#include <string>
#include <list>
#include "datatype.h"

namespace sql
{
class ResultSet;

class DatabaseMetaData
{
protected:
	virtual ~DatabaseMetaData() {}

public:
	enum
	{
		attributeNoNulls = 0,
		attributeNullable,
		attributeNullableUnknown
	};
	enum
	{
		bestRowTemporary = 0,
		bestRowTransaction,
		bestRowSession
	};
	enum
	{
		bestRowUnknown = 0,
		bestRowNotPseudo,
		bestRowPseudo
	};
	enum
	{
		columnNoNulls = 0,
		columnNullable,
		columnNullableUnknown
	};
	enum
	{
		importedKeyCascade = 0,
		importedKeyInitiallyDeferred,
		importedKeyInitiallyImmediate,
		importedKeyNoAction,
		importedKeyNotDeferrable,
		importedKeyRestrict,
		importedKeySetDefault,
		importedKeySetNull
	};
	enum
	{
		procedureColumnIn = 0,
		procedureColumnInOut,
		procedureColumnOut,
		procedureColumnResult,
		procedureColumnReturn,
		procedureColumnUnknown,
		procedureNoNulls,
		procedureNoResult,
		procedureNullable,
		procedureNullableUnknown,
		procedureResultUnknown,
		procedureReturnsResult
	};
	enum
	{
		sqlStateSQL99 = 0,
		sqlStateXOpen
	};
	enum
	{
		tableIndexClustered = 0,
		tableIndexHashed,
		tableIndexOther,
		tableIndexStatistic
	};
	enum
	{
		versionColumnUnknown = 0,
		versionColumnNotPseudo = 1,
		versionColumnPseudo = 2
	};
	enum
	{
		typeNoNulls = 0,
		typeNullable = 1,
		typeNullableUnknown = 2
	};
	enum
	{
		typePredNone = 0,
		typePredChar = 1,
		typePredBasic= 2,
		typeSearchable = 3
	};


	virtual	bool allProceduresAreCallable() = 0;

	virtual	bool allTablesAreSelectable() = 0;

	virtual	bool dataDefinitionCausesTransactionCommit() = 0;

	virtual	bool dataDefinitionIgnoredInTransactions() = 0;

	virtual	bool deletesAreDetected(int type) = 0;

	virtual	bool doesMaxRowSizeIncludeBlobs() = 0;

	virtual	ResultSet * getAttributes(const std::string& catalog, const std::string& schemaPattern, const std::string& typeNamePattern, const std::string& attributeNamePattern) = 0;

	virtual	ResultSet * getBestRowIdentifier(const std::string& catalog, const std::string& schema, const std::string& table, int scope, bool nullable) = 0;

	virtual	ResultSet * getCatalogs() = 0;

	virtual	const std::string& getCatalogSeparator() = 0;

	virtual	const std::string& getCatalogTerm() = 0;

	virtual	ResultSet * getColumnPrivileges(const std::string& catalog, const std::string& schema, const std::string& table, const std::string& columnNamePattern) = 0;

	virtual	ResultSet * getColumns(const std::string& catalog, const std::string& schemaPattern, const std::string& tableNamePattern, const std::string& columnNamePattern) = 0;

	virtual	Connection * getConnection() = 0;

	virtual	ResultSet * getCrossReference(const std::string& primaryCatalog, const std::string& primarySchema, const std::string& primaryTable, const std::string& foreignCatalog, const std::string& foreignSchema, const std::string& foreignTable) = 0;

	virtual	unsigned int getDatabaseMajorVersion() = 0;

	virtual	unsigned int getDatabaseMinorVersion() = 0;

	virtual	unsigned int getDatabasePatchVersion() = 0;

	virtual	const std::string& getDatabaseProductName() = 0;

	virtual	std::string getDatabaseProductVersion() = 0;

	virtual	int getDefaultTransactionIsolation() = 0;

	virtual	unsigned int getDriverMajorVersion() = 0;

	virtual	unsigned int getDriverMinorVersion() = 0;

	virtual	unsigned int getDriverPatchVersion() = 0;

	virtual	const std::string& getDriverName() = 0;

	virtual	const std::string& getDriverVersion() = 0;

	virtual	ResultSet * getExportedKeys(const std::string& catalog, const std::string& schema, const std::string& table) = 0;

	virtual	const std::string& getExtraNameCharacters() = 0;

	virtual	const std::string& getIdentifierQuoteString() = 0;

	virtual	ResultSet * getImportedKeys(const std::string& catalog, const std::string& schema, const std::string& table) = 0;

	virtual	ResultSet * getIndexInfo(const std::string& catalog, const std::string& schema, const std::string& table, bool unique, bool approximate) = 0;

	virtual	unsigned int getCDBCMajorVersion() = 0;

	virtual	unsigned int getCDBCMinorVersion() = 0;

	virtual	unsigned int getMaxBinaryLiteralLength() = 0;

	virtual	unsigned int getMaxCatalogNameLength() = 0;

	virtual	unsigned int getMaxCharLiteralLength() = 0;

	virtual	unsigned int getMaxColumnNameLength() = 0;

	virtual	unsigned int getMaxColumnsInGroupBy() = 0;

	virtual	unsigned int getMaxColumnsInIndex() = 0;

	virtual	unsigned int getMaxColumnsInOrderBy() = 0;

	virtual	unsigned int getMaxColumnsInSelect() = 0;

	virtual	unsigned int getMaxColumnsInTable() = 0;

	virtual	unsigned int getMaxConnections() = 0;

	virtual	unsigned int getMaxCursorNameLength() = 0;

	virtual	unsigned int getMaxIndexLength() = 0;

	virtual	unsigned int getMaxProcedureNameLength() = 0;

	virtual	unsigned int getMaxRowSize() = 0;

	virtual	unsigned int getMaxSchemaNameLength() = 0;

	virtual	unsigned int getMaxStatementLength() = 0;

	virtual	unsigned int getMaxStatements() = 0;

	virtual	unsigned int getMaxTableNameLength() = 0;

	virtual	unsigned int getMaxTablesInSelect() = 0;

	virtual	unsigned int getMaxUserNameLength() = 0;

	virtual	const std::string& getNumericFunctions() = 0;

	virtual	ResultSet * getPrimaryKeys(const std::string& catalog, const std::string& schema, const std::string& table) = 0;

	virtual	ResultSet * getProcedures(const std::string& catalog, const std::string& schemaPattern, const std::string& procedureNamePattern) = 0;

	virtual	const std::string& getProcedureTerm() = 0;

	virtual	int getResultSetHoldability() = 0;

	virtual	ResultSet * getSchemas() = 0;

	virtual	const std::string& getSchemaTerm() = 0;

	virtual	const std::string& getSearchStringEscape() = 0;

	virtual	const std::string& getSQLKeywords() = 0;

	virtual	int getSQLStateType() = 0;

	virtual const std::string& getStringFunctions() = 0;

	virtual	ResultSet * getSuperTables(const std::string& catalog, const std::string& schemaPattern, const std::string& tableNamePattern) = 0;

	virtual	ResultSet * getSuperTypes(const std::string& catalog, const std::string& schemaPattern, const std::string& typeNamePattern) = 0;

	virtual	const std::string& getSystemFunctions() = 0;

	virtual	ResultSet * getTablePrivileges(const std::string& catalog, const std::string& schemaPattern, const std::string& tableNamePattern) = 0;

	virtual	ResultSet * getTables(const std::string& catalog, const std::string& schemaPattern, const std::string& tableNamePattern, std::list<std::string> &types) = 0;

	virtual	ResultSet * getTableTypes() = 0;

	virtual	const std::string& getTimeDateFunctions() = 0;

	virtual	ResultSet * getTypeInfo() = 0;

	virtual	ResultSet * getUDTs(const std::string& catalog, const std::string& schemaPattern, const std::string& typeNamePattern, std::list<int> &types) = 0;

	virtual std::string getUserName() = 0;

	virtual ResultSet * getVersionColumns(const std::string& catalog, const std::string& schema, const std::string& table) = 0;

	virtual bool insertsAreDetected(int type) = 0;

	virtual bool isCatalogAtStart() = 0;

	virtual bool isReadOnly() = 0;

	virtual bool nullPlusNonNullIsNull() = 0;

	virtual bool nullsAreSortedAtEnd() = 0;

	virtual bool nullsAreSortedAtStart() = 0;

	virtual bool nullsAreSortedHigh() = 0;

	virtual bool nullsAreSortedLow() = 0;

	virtual bool othersDeletesAreVisible(int type) = 0;

	virtual bool othersInsertsAreVisible(int type) = 0;

	virtual bool othersUpdatesAreVisible(int type) = 0;

	virtual bool ownDeletesAreVisible(int type) = 0;

	virtual bool ownInsertsAreVisible(int type) = 0;

	virtual bool ownUpdatesAreVisible(int type) = 0;

	virtual bool storesLowerCaseIdentifiers() = 0;

	virtual bool storesLowerCaseQuotedIdentifiers() = 0;

	virtual bool storesMixedCaseIdentifiers() = 0;

	virtual bool storesMixedCaseQuotedIdentifiers() = 0;

	virtual bool storesUpperCaseIdentifiers() = 0;

	virtual bool storesUpperCaseQuotedIdentifiers() = 0;

	virtual bool supportsAlterTableWithAddColumn() = 0;

	virtual bool supportsAlterTableWithDropColumn() = 0;

	virtual bool supportsANSI92EntryLevelSQL() = 0;

	virtual bool supportsANSI92FullSQL() = 0;

	virtual bool supportsANSI92IntermediateSQL() = 0;

	virtual bool supportsBatchUpdates() = 0;

	virtual bool supportsCatalogsInDataManipulation() = 0;

	virtual bool supportsCatalogsInIndexDefinitions() = 0;

	virtual bool supportsCatalogsInPrivilegeDefinitions() = 0;

	virtual bool supportsCatalogsInProcedureCalls() = 0;

	virtual bool supportsCatalogsInTableDefinitions() = 0;

	virtual bool supportsColumnAliasing() = 0;

	virtual bool supportsConvert() = 0;

	virtual bool supportsConvert(int fromType, int toType) = 0;

	virtual bool supportsCoreSQLGrammar() = 0;

	virtual bool supportsCorrelatedSubqueries() = 0;

	virtual bool supportsDataDefinitionAndDataManipulationTransactions() = 0;

	virtual bool supportsDataManipulationTransactionsOnly() = 0;

	virtual bool supportsDifferentTableCorrelationNames() = 0;

	virtual bool supportsExpressionsInOrderBy() = 0;

	virtual bool supportsExtendedSQLGrammar() = 0;

	virtual bool supportsFullOuterJoins() = 0;

	virtual bool supportsGetGeneratedKeys() = 0;

	virtual bool supportsGroupBy() = 0;

	virtual bool supportsGroupByBeyondSelect() = 0;

	virtual bool supportsGroupByUnrelated() = 0;

	virtual bool supportsLikeEscapeClause() = 0;

	virtual bool supportsLimitedOuterJoins() = 0;

	virtual bool supportsMinimumSQLGrammar() = 0;

	virtual bool supportsMixedCaseIdentifiers() = 0;

	virtual bool supportsMixedCaseQuotedIdentifiers() = 0;

	virtual bool supportsMultipleOpenResults() = 0;

	virtual bool supportsMultipleResultSets() = 0;

	virtual bool supportsMultipleTransactions() = 0;

	virtual bool supportsNamedParameters() = 0;

	virtual bool supportsNonNullableColumns() = 0;

	virtual bool supportsOpenCursorsAcrossCommit() = 0;

	virtual bool supportsOpenCursorsAcrossRollback() = 0;

	virtual bool supportsOpenStatementsAcrossCommit() = 0;

	virtual bool supportsOpenStatementsAcrossRollback() = 0;

	virtual bool supportsOrderByUnrelated() = 0;

	virtual bool supportsOuterJoins() = 0;

	virtual bool supportsPositionedDelete() = 0;

	virtual bool supportsPositionedUpdate() = 0;

	virtual bool supportsResultSetHoldability(int holdability) = 0;

	virtual bool supportsResultSetType(int type) = 0;

	virtual bool supportsSavepoints() = 0;

	virtual bool supportsSchemasInDataManipulation() = 0;

	virtual bool supportsSchemasInIndexDefinitions() = 0;

	virtual bool supportsSchemasInPrivilegeDefinitions() = 0;

	virtual bool supportsSchemasInProcedureCalls() = 0;

	virtual bool supportsSchemasInTableDefinitions() = 0;

	virtual bool supportsSelectForUpdate() = 0;

	virtual bool supportsStatementPooling() = 0;

	virtual bool supportsStoredProcedures() = 0;

	virtual bool supportsSubqueriesInComparisons() = 0;

	virtual bool supportsSubqueriesInExists() = 0;

	virtual bool supportsSubqueriesInIns() = 0;

	virtual bool supportsSubqueriesInQuantifieds() = 0;

	virtual bool supportsTableCorrelationNames() = 0;

	virtual bool supportsTransactionIsolationLevel(int level) = 0;

	virtual bool supportsTransactions() = 0;

	virtual bool supportsTypeConversion() = 0; /* SDBC */

	virtual bool supportsUnion() = 0;

	virtual bool supportsUnionAll() = 0;

	virtual bool updatesAreDetected(int type) = 0;

	virtual bool usesLocalFilePerTable() = 0;

	virtual bool usesLocalFiles() = 0;

	virtual ResultSet *getSchemata(const std::string& catalogName = "") = 0;

	virtual ResultSet *getSchemaObjects(const std::string& catalogName = "",
										const std::string& schemaName = "",
										const std::string& objectType = "") = 0;

	virtual ResultSet *getSchemaObjectTypes() = 0;
};


} /* namespace sql */

#endif /* _SQL_METADATA_H_ */
