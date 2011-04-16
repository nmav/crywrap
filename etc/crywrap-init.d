#! /bin/sh
## /etc/init.d/crywrap -- init script for CryWrap
## (Generic, used for Debian)
##
## arch-tag: 95d90d0c-199e-4f43-9bf3-33706a009a19

set -e

CRYWRAP=/usr/sbin/crywrap
CONFFILE=/etc/default/crywrap
PIDDIR=/var/run/crywrap

test -x ${CRYWRAP} || exit 0

## Ignore starts here..
# crywrap_map_add dest listen
crywrap_map_add ()
{
	SRC_PORT="$(echo $1 | sed -e 's,^.*/,,g')"
	SRC_IP="$(echo $1 | sed -e 's,/.*,,g')"
	DST_PORT="$(echo $2 | sed -e 's,^.*/,,g')"
	DST_IP="$(echo $2 | sed -e 's,/.*,,g')"

	CRYWRAP_MAP="${CRYWRAP_MAP}-d ${SRC_IP}/${SRC_PORT} -l ${DST_IP}/${DST_PORT}|"
}
CRYWRAP_MAP=
## Unignore starts here :)

[ -e ${CONFFILE} ] && . ${CONFFILE}

crywrap_start ()
{
	install -d -m 1777 ${PIDDIR}
	cnt=0
	IFS_SAVE="${IFS}"
	IFS="|"
	set -- ${CRYWRAP_MAP}
	while [ "$#" -gt 0 ]; do
		PC=""; PK=""
		if [ ! -z "${CRYWRAP_CERTFILE}" ]; then
			PC="cert=${CRYWRAP_CERTFILE}"
		fi
		if [ ! -z "${CRYWRAP_KEYFILE}" ]; then
			PK="key=${CRYWRAP_KEYFILE}"
		fi
		P="${PC}${PC:+,}${PK}"
		P="${P:+-p ${P}}"
		CMDLINE="$1 -P ${PIDDIR}/crywrap-${cnt}.pid ${P} \
			 ${CRYWRAP_USER:+-u ${CRYWRAP_USER}} \
			 ${CRYWRAP_OPTIONS}"
		cnt=$(expr ${cnt} + 1)
		eval ${CRYWRAP} ${CMDLINE}
		echo -n "."
		shift
	done
	IFS="${IFS_SAVE}"
	if [ "$cnt" -eq 0 ]; then
		echo " - not started"
	else
		echo " done."
	fi
}

crywrap_stop ()
{
	for PIDFILE in ${PIDDIR}/*; do
		if [ -e ${PIDFILE} ]; then
			PID=$(cat ${PIDFILE})
			if [ ! -z "${PID}" ]; then
				if kill -15 ${PID} >/dev/null 2>&1; then
					echo -n "."
				fi
			fi
		fi
		rm -f ${PIDFILE}
	done
	rm -rf ${PIDDIR}
}

case $1 in
	start)
		echo -n "Starting TLS wrapper: crywrap"
		crywrap_start;;
	stop)
		echo -n "Stopping TLS wrapper: crywrap"
		crywrap_stop 
		echo ".";;
	restart|force-reload)
		$0 stop
		sleep 2
		$0 start;;
	*)
		echo "Usage: /etc/init.d/crywrap {start|stop|restart|force-reload}"
		exit 1;;
esac

exit 0
