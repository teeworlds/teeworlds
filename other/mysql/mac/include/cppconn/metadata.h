/*
  Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.

  The MySQL Connector/C++ is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _SQL_METADATA_H_
#define _SQL_METADATA_H_

#include <string>
#include <list>
#include "datatype.h"
#include "sqlstring.h"

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

	virtual	ResultSet * getAttributes(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& typeNamePattern, const sql::SQLString& attributeNamePattern) = 0;

	virtual	ResultSet * getBestRowIdentifier(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table, int scope, bool nullable) = 0;

	virtual	ResultSet * getCatalogs() = 0;

	virtual	const sql::SQLString& getCatalogSeparator() = 0;

	virtual	const sql::SQLString& getCatalogTerm() = 0;

	virtual	ResultSet * getColumnPrivileges(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table, const sql::SQLString& columnNamePattern) = 0;

	virtual	ResultSet * getColumns(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& tableNamePattern, const sql::SQLString& columnNamePattern) = 0;

	virtual	Connection * getConnection() = 0;

	virtual	ResultSet * getCrossReference(const sql::SQLString& primaryCatalog, const sql::SQLString& primarySchema, const sql::SQLString& primaryTable, const sql::SQLString& foreignCatalog, const sql::SQLString& foreignSchema, const sql::SQLString& foreignTable) = 0;

	virtual	unsigned int getDatabaseMajorVersion() = 0;

	virtual	unsigned int getDatabaseMinorVersion() = 0;

	virtual	unsigned int getDatabasePatchVersion() = 0;

	virtual	const sql::SQLString& getDatabaseProductName() = 0;

	virtual	SQLString getDatabaseProductVersion() = 0;

	virtual	int getDefaultTransactionIsolation() = 0;

	virtual	unsigned int getDriverMajorVersion() = 0;

	virtual	unsigned int getDriverMinorVersion() = 0;

	virtual	unsigned int getDriverPatchVersion() = 0;

	virtual	const sql::SQLString& getDriverName() = 0;

	virtual	const sql::SQLString& getDriverVersion() = 0;

	virtual	ResultSet * getExportedKeys(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table) = 0;

	virtual	const sql::SQLString& getExtraNameCharacters() = 0;

	virtual	const sql::SQLString& getIdentifierQuoteString() = 0;

	virtual	ResultSet * getImportedKeys(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table) = 0;

	virtual	ResultSet * getIndexInfo(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table, bool unique, bool approximate) = 0;

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

	virtual	const sql::SQLString& getNumericFunctions() = 0;

	virtual	ResultSet * getPrimaryKeys(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table) = 0;

	virtual	ResultSet * getProcedureColumns(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& procedureNamePattern, const sql::SQLString& columnNamePattern) = 0;

	virtual	ResultSet * getProcedures(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& procedureNamePattern) = 0;

	virtual	const sql::SQLString& getProcedureTerm() = 0;

	virtual	int getResultSetHoldability() = 0;

	virtual	ResultSet * getSchemas() = 0;

	virtual	const sql::SQLString& getSchemaTerm() = 0;

	virtual	const sql::SQLString& getSearchStringEscape() = 0;

	virtual	const sql::SQLString& getSQLKeywords() = 0;

	virtual	int getSQLStateType() = 0;

	virtual const sql::SQLString& getStringFunctions() = 0;

	virtual	ResultSet * getSuperTables(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& tableNamePattern) = 0;

	virtual	ResultSet * getSuperTypes(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& typeNamePattern) = 0;

	virtual	const sql::SQLString& getSystemFunctions() = 0;

	virtual	ResultSet * getTablePrivileges(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& tableNamePattern) = 0;

	virtual	ResultSet * getTables(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& tableNamePattern, std::list<sql::SQLString> &types) = 0;

	virtual	ResultSet * getTableTypes() = 0;

	virtual	const sql::SQLString& getTimeDateFunctions() = 0;

	virtual	ResultSet * getTypeInfo() = 0;

	virtual	ResultSet * getUDTs(const sql::SQLString& catalog, const sql::SQLString& schemaPattern, const sql::SQLString& typeNamePattern, std::list<int> &types) = 0;

	virtual	SQLString getURL() = 0;

	virtual SQLString getUserName() = 0;

	virtual ResultSet * getVersionColumns(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table) = 0;

	virtual bool insertsAreDetected(int type) = 0;

	virtual bool isCatalogAtStart() = 0;

	virtual bool isReadOnly() = 0;

	virtual bool locatorsUpdateCopy() = 0;

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

	virtual bool supportsIntegrityEnhancementFacility() = 0;

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

	virtual bool supportsResultSetConcurrency(int type, int concurrency) = 0;

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

	virtual ResultSet *getSchemata(const sql::SQLString& catalogName = "") = 0;

	virtual ResultSet *getSchemaObjects(const sql::SQLString& catalogName = "",
										const sql::SQLString& schemaName = "",
										const sql::SQLString& objectType = "",
										bool includingDdl = true,
										const sql::SQLString& objectName = "",
										const sql::SQLString& contextTableName = "") = 0;

	virtual ResultSet *getSchemaObjectTypes() = 0;
};


} /* namespace sql */

#endif /* _SQL_METADATA_H_ */
