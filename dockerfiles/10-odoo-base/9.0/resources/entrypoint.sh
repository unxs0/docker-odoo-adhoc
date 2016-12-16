#!/bin/bash
# from odoo entrypoint
set -e

# set odoo database host, port, user and password
: ${PGHOST:=$DB_PORT_5432_TCP_ADDR}
: ${PGPORT:=$DB_PORT_5432_TCP_PORT}
: ${PGUSER:=${DB_ENV_POSTGRES_USER:='postgres'}}
: ${PGPASSWORD:=$DB_ENV_POSTGRES_PASSWORD}
export PGHOST PGPORT PGUSER PGPASSWORD

# copy_sources
function copy_sources {
    echo "Making a copy of Extra Addons to Custom addons"
    cp -R $EXTRA_ADDONS/* $CUSTOM_ADDONS
}

if [ "$*" == "copy_sources" ]; then
    copy_sources
    exit 1
fi

# Ensure proper content for $UNACCENT
if [ "$UNACCENT" != "True" ]; then
    UNACCENT=False
fi
# we add sort to find so ingadhoc paths are located before the others and prefered by odoo
echo Patching configuration > /dev/stderr
addons=$(find $CUSTOM_ADDONS $EXTRA_ADDONS -mindepth 1 -maxdepth 1 -type d | sort | tr '\n' ',')

echo "
[options]
; Configuration file generated by $(readlink --canonicalize $0)
addons_path = " $addons "
unaccent = $UNACCENT
workers = $WORKERS
db_user = $PGUSER
db_password = $PGPASSWORD
db_host = $PGHOST
db_template = $DB_TEMPLATE
admin_passwd = $ADMIN_PASSWORD
data_dir = $DATA_DIR
proxy_mode = $PROXY_MODE
without_demo = $WITHOUT_DEMO
server_mode = $SERVER_MODE
server_wide_modules = $SERVER_WIDE_MODULES
# auto_reload = True

# other performance parameters
# db_maxconn = 32
# limit_memory_hard = 2684354560
# limit_memory_soft = 2147483648
# limit_request = 8192
# limit_time_cpu = 600
# limit_time_real = 1200

# aeroo config
aeroo.docs_enabled = True
aeroo.docs_host = $AEROO_DOCS_HOST

# afip certificates
afip_homo_pkey_file = $AFIP_HOMO_PKEY_FILE
afip_homo_cert_file = $AFIP_HOMO_CERT_FILE
afip_prod_pkey_file = $AFIP_PROD_PKEY_FILE
afip_prod_cert_file = $AFIP_PROD_CERT_FILE
" > $ODOO_CONF

# If database is available, use it
if [ "$DATABASE" != "" ]; then
    echo "database = $DATABASE" >> $ODOO_CONF
fi

# Know if Postgres is listening
function db_is_listening() {
    psql --list > /dev/null 2>&1 || (sleep 1 && db_is_listening)
}

echo Waiting until the database server is listening... > /dev/stderr
db_is_listening

# Check pg user exist
function pg_user_exist() {
    psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$PGUSER'" > /dev/null 2>&1 || (sleep 1 && pg_user_exist)
}

echo Waiting until the pg user $PGUSER is created... > /dev/stderr
pg_user_exist

# Add the unaccent module for the database if needed
if [ "$UNACCENT" == "True" ]; then
    echo Trying to install unaccent extension > /dev/stderr
    psql -d $DB_TEMPLATE -c 'CREATE EXTENSION IF NOT EXISTS unaccent;'
fi

# Run server
echo "Running command..."
case "$1" in
    --)
        shift
        exec $ODOO_SERVER "$@"
        ;;
    -*)
        exec $ODOO_SERVER "$@"
        ;;
    *)
        exec "$@"
esac

exit 1
