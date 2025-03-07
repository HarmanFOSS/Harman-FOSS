# Copyright (C) - 2012 David Goulet <dgoulet@efficios.com>
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

SESSIOND_BIN="lttng-sessiond"
SESSIOND_MATCH=".*lttng-sess.*"
RUNAS_BIN="lttng-runas"
RUNAS_MATCH=".*lttng-runas.*"
CONSUMERD_BIN="lttng-consumerd"
CONSUMERD_MATCH=".*lttng-consumerd.*"
RELAYD_BIN="lttng-relayd"
RELAYD_MATCH=".*lttng-relayd.*"
LTTNG_BIN="lttng"
BABELTRACE_BIN="babeltrace"
OUTPUT_DEST=/dev/null
ERROR_OUTPUT_DEST=/dev/null

# Minimal kernel version supported for session daemon tests
KERNEL_MAJOR_VERSION=2
KERNEL_MINOR_VERSION=6
KERNEL_PATCHLEVEL_VERSION=27

# We set the default UST register timeout and network and app socket timeout to
# "wait forever", so that basic tests don't have to worry about hitting
# timeouts on busy systems. Specialized tests should test those corner-cases.
export LTTNG_UST_REGISTER_TIMEOUT=-1
export LTTNG_NETWORK_SOCKET_TIMEOUT=-1
export LTTNG_APP_SOCKET_TIMEOUT=-1

# We set the default lttng-sessiond path to /bin/true to prevent the spawning
# of a daemonized sessiond. This is necessary since 'lttng create' will spawn
# its own sessiond if none is running. It also ensures that 'lttng create'
# fails when no sessiond is running.
export LTTNG_SESSIOND_PATH="/bin/true"

source $TESTDIR/utils/tap/tap.sh

if [ -z $LTTNG_TEST_TEARDOWN_TIMEOUT ]; then
	LTTNG_TEST_TEARDOWN_TIMEOUT=60
fi

function full_cleanup ()
{
	# Try to kill daemons gracefully
	stop_lttng_relayd_notap SIGTERM $LTTNG_TEST_TEARDOWN_TIMEOUT
	stop_lttng_sessiond_notap SIGTERM $LTTNG_TEST_TEARDOWN_TIMEOUT

	# If daemons are still present, forcibly kill them
	stop_lttng_relayd_notap SIGKILL $LTTNG_TEST_TEARDOWN_TIMEOUT
	stop_lttng_sessiond_notap SIGKILL $LTTNG_TEST_TEARDOWN_TIMEOUT
	stop_lttng_consumerd_notap SIGKILL $LTTNG_TEST_TEARDOWN_TIMEOUT

	# Disable trap for SIGTERM since the following kill to the
	# pidgroup will be SIGTERM. Otherwise it loops.
	# The '-' before the pid number ($$) indicates 'kill' to signal the
	# whole process group.
	trap - SIGTERM && kill -- -$$
	exit 1
}

function null_pipes ()
{
	exec 0>/dev/null
	exec 1>/dev/null
	exec 2>/dev/null
}

trap full_cleanup SIGINT SIGTERM

# perl prove closes its child pipes before giving it a chance to run its
# signal trap handlers. Redirect pipes to /dev/null if SIGPIPE is caught
# to allow those trap handlers to proceed.

trap null_pipes SIGPIPE

function print_ok ()
{
	# Check if we are a terminal
	if [ -t 1 ]; then
		echo -e "\e[1;32mOK\e[0m"
	else
		echo -e "OK"
	fi
}

function print_fail ()
{
	# Check if we are a terminal
	if [ -t 1 ]; then
		echo -e "\e[1;31mFAIL\e[0m"
	else
		echo -e "FAIL"
	fi
}

function print_test_banner ()
{
	local desc="$1"
	diag "$desc"
}

function validate_kernel_version ()
{
	local kern_version=($(uname -r | awk -F. '{ printf("%d.%d.%d\n",$1,$2,$3); }' | tr '.' '\n'))
	if [ ${kern_version[0]} -gt $KERNEL_MAJOR_VERSION ]; then
		return 0
	fi
	if [ ${kern_version[1]} -gt $KERNEL_MINOR_VERSION ]; then
		return 0
	fi
	if [ ${kern_version[2]} -ge $KERNEL_PATCHLEVEL_VERSION ]; then
		return 0
	fi
	return 1
}

# Generate a random string
#  $1 = number of characters; defaults to 16
#  $2 = include special characters; 1 = yes, 0 = no; defaults to yes
function randstring()
{
	[ "$2" == "0" ] && CHAR="[:alnum:]" || CHAR="[:graph:]"
	cat /dev/urandom 2>/dev/null | tr -cd "$CHAR" 2>/dev/null | head -c ${1:-16} 2>/dev/null
	echo
}

# Return the number of _configured_ CPUs.
function conf_proc_count()
{
	getconf _NPROCESSORS_CONF
	if [ $? -ne 0 ]; then
		diag "Failed to get the number of configured CPUs"
	fi
	echo
}

# Check if base lttng-modules are present.
# Bail out on failure
function validate_lttng_modules_present ()
{
	modprobe -n lttng-tracer 2>/dev/null
	if [ $? -ne 0  ]; then
		BAIL_OUT "LTTng modules not detected."
	fi
}

function enable_kernel_lttng_event
{
	local withtap="$1"
	local expected_to_fail="$2"
	local sess_name="$3"
	local event_name="$4"
	local channel_name="$5"

	if [ -z "$event_name" ]; then
		# Enable all event if no event name specified
		event_name="-a"
	fi

	if [ -z "$channel_name" ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" $chan -s $sess_name -k 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable kernel event $event_name for session $session_name on channel $channel_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable kernel event $event_name for session $sess_name"
		fi
	fi
}

function enable_kernel_lttng_event_ok ()
{
	enable_kernel_lttng_event 1 0 "$@"
}

function enable_kernel_lttng_event_fail ()
{
	enable_kernel_lttng_event 1 1 "$@"
}

function enable_kernel_lttng_event_notap ()
{
	enable_kernel_lttng_event 0 0 "$@"
}

# Old interface
function lttng_enable_kernel_event
{
	enable_kernel_lttng_event_ok "$@"
}

function lttng_enable_kernel_syscall()
{
	local expected_to_fail=$1
	local sess_name=$2
	local syscall_name=$3
	local channel_name=$4

	if [ -z $syscall_name ]; then
		# Enable all event if no syscall name specified
		syscall_name="-a"
	fi

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event --syscall "$syscall_name" $chan -s $sess_name -k 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Enable kernel syscall $syscall_name for session $sess_name on channel $channel_name fail as expected"
	else
		ok $ret "Enable kernel syscall $syscall_name for session $sess_name on channel $channel_name"
	fi
}

function lttng_enable_kernel_syscall_ok()
{
	lttng_enable_kernel_syscall 0 "$@"
}

function lttng_enable_kernel_syscall_fail()
{
	lttng_enable_kernel_syscall 1 "$@"
}

function lttng_disable_kernel_syscall()
{
	local expected_to_fail=$1
	local sess_name=$2
	local syscall_name=$3
	local channel_name=$4

	if [ -z $syscall_name ]; then
		# Enable all event if no syscall name specified
		syscall_name="-a"
	fi

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-event --syscall "$syscall_name" $chan -s $sess_name -k 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST

	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Disable kernel syscall $syscall_name for session $sess_name on channel $channel_name failed as expected"
	else
		ok $ret "Disable kernel syscall $syscall_name for session $sess_name on channel $channel_name"
	fi
}

function lttng_disable_kernel_syscall_ok()
{
	lttng_disable_kernel_syscall 0 "$@"
}

function lttng_disable_kernel_syscall_fail()
{
	lttng_disable_kernel_syscall 1 "$@"
}

function lttng_enable_kernel_userspace_probe_event ()
{
	local expected_to_fail="$1"
	local sess_name="$2"
	local target="$3"
	local event_name="$4"

	"$TESTDIR/../src/bin/lttng/$LTTNG_BIN" enable-event --kernel --userspace-probe="$target" "$event_name" -s "$sess_name" > "$OUTPUT_DEST" 2> "$ERROR_OUTPUT_DEST"
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Enable kernel userspace probe event for session $sess_name failed as expected"
	else
		ok $ret "Enable kernel userspace probe event for session $sess_name"
	fi
}

function lttng_enable_kernel_userspace_probe_event_fail ()
{
	lttng_enable_kernel_userspace_probe_event 1 "$@"
}

function lttng_enable_kernel_userspace_probe_event_ok ()
{
	lttng_enable_kernel_userspace_probe_event 0 "$@"
}

function disable_kernel_lttng_userspace_probe_event_ok ()
{
	local sess_name="$1"
	local event_name="$2"

	"$TESTDIR/../src/bin/lttng/$LTTNG_BIN" disable-event --kernel "$event_name" -s "$sess_name" > "$OUTPUT_DEST" 2> "$ERROR_OUTPUT_DEST"
	ok $? "Disable kernel event $target for session $sess_name"
}
function lttng_enable_kernel_channel()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3
	local channel_name=$4
	local opt=$5

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -k $channel_name -s $sess_name $opt 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable channel $channel_name for session $sess_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable channel $channel_name for session $sess_name"
		fi
	fi
}

function lttng_enable_kernel_channel_ok()
{
	lttng_enable_kernel_channel 1 0 "$@"
}

function lttng_enable_kernel_channel_fail()
{
	lttng_enable_kernel_channel 1 1 "$@"
}

function lttng_enable_kernel_channel_notap()
{
	lttng_enable_kernel_channel 0 0 "$@"
}

function enable_kernel_lttng_channel_ok()
{
	lttng_enable_kernel_channel 1 0 "$@"
}

function lttng_disable_kernel_channel()
{
	local expected_to_fail=$1
	local sess_name=$2
	local channel_name=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-channel -k $channel_name -s $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Disable channel $channel_name for session $sess_name failed as expected"
	else
		ok $ret "Disable channel $channel_name for session $sess_name"
	fi
}

function lttng_disable_kernel_channel_ok()
{
	lttng_disable_kernel_channel 0 "$@"
}

function lttng_disable_kernel_channel_fail()
{
	lttng_disable_kernel_channel 1 "$@"
}

function start_lttng_relayd_opt()
{
	local withtap=$1
	local opt=$2

	DIR=$(readlink -f "$TESTDIR")

	if [ -z "$(pgrep "$RELAYD_MATCH")" ]; then
		# shellcheck disable=SC2086
		"$DIR/../src/bin/lttng-relayd/$RELAYD_BIN" -b $opt 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
		#"$DIR/../src/bin/lttng-relayd/$RELAYD_BIN" $opt -vvv >>/tmp/relayd.log 2>&1 &
		if [ $? -eq 1 ]; then
			if [ "$withtap" -eq "1" ]; then
				fail "Start lttng-relayd (opt: $opt)"
			fi
			return 1
		else
			if [ "$withtap" -eq "1" ]; then
				pass "Start lttng-relayd (opt: $opt)"
			fi
		fi
	else
		pass "Start lttng-relayd (opt: $opt)"
	fi
}

function start_lttng_relayd()
{
	start_lttng_relayd_opt 1 "$@"
}

function start_lttng_relayd_notap()
{
	start_lttng_relayd_opt 0 "$@"
}

function stop_lttng_relayd_opt()
{
	local withtap=$1
	local signal=$2

	if [ -z "$signal" ]; then
		signal="SIGTERM"
	fi

	local timeout_s=$3
	local dtimeleft_s=

	# Multiply time by 2 to simplify integer arithmetic
	if [ -n "$timeout_s" ]; then
		dtimeleft_s=$((timeout_s * 2))
	fi

	local retval=0
	local pids=

	pids=$(pgrep "$RELAYD_MATCH")
	if [ -z "$pids" ]; then
		if [ "$withtap" -eq "1" ]; then
			pass "No relay daemon to kill"
		fi
		return 0
	fi

	diag "Killing (signal $signal) lttng-relayd (pid: $pids)"

	# shellcheck disable=SC2086
	if ! kill -s $signal $pids 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST; then
		retval=1
		if [ "$withtap" -eq "1" ]; then
			fail "Kill relay daemon"
		fi
	else
		out=1
		while [ -n "$out" ]; do
			out=$(pgrep "$RELAYD_MATCH")
			if [ -n "$dtimeleft_s" ]; then
				if [ $dtimeleft_s -lt 0 ]; then
					out=
					retval=1
				fi
				dtimeleft_s=$((dtimeleft_s - 1))
			fi
			sleep 0.5
		done
		if [ "$withtap" -eq "1" ]; then
			if [ "$retval" -eq "0" ]; then
				pass "Wait after kill relay daemon"
			else
				fail "Wait after kill relay daemon"
			fi
		fi
	fi
	return $retval
}

function stop_lttng_relayd()
{
	stop_lttng_relayd_opt 1 "$@"
}

function stop_lttng_relayd_notap()
{
	stop_lttng_relayd_opt 0 "$@"
}

#First arg: show tap output
#Second argument: load path for automatic loading
function start_lttng_sessiond_opt()
{
	local withtap=$1
	local load_path=$2

	local env_vars=""
	local consumerd=""

	local long_bit_value=
	long_bit_value=$(getconf LONG_BIT)

	if [ -n "$TEST_NO_SESSIOND" ] && [ "$TEST_NO_SESSIOND" == "1" ]; then
		# Env variable requested no session daemon
		return
	fi

	DIR=$(readlink -f "$TESTDIR")

	# Get long_bit value for 32/64 consumerd
	case "$long_bit_value" in
		32)
			consumerd="--consumerd32-path=$DIR/../src/bin/lttng-consumerd/lttng-consumerd"
			;;
		64)
			consumerd="--consumerd64-path=$DIR/../src/bin/lttng-consumerd/lttng-consumerd"
			;;
		*)
			return
			;;
	esac

	# Check for env. variable. Allow the use of LD_PRELOAD etc.
	if [[ "x${LTTNG_SESSIOND_ENV_VARS}" != "x" ]]; then
		env_vars="${LTTNG_SESSIOND_ENV_VARS} "
	fi
	env_vars="${env_vars}$DIR/../src/bin/lttng-sessiond/$SESSIOND_BIN"

	if ! validate_kernel_version; then
	    fail "Start session daemon"
	    BAIL_OUT "*** Kernel too old for session daemon tests ***"
	fi

	: "${LTTNG_SESSION_CONFIG_XSD_PATH="${DIR}/../src/common/config/"}"
	export LTTNG_SESSION_CONFIG_XSD_PATH

	if [ -z "$(pgrep "${SESSIOND_MATCH}")" ]; then
		# Have a load path ?
		if [ -n "$load_path" ]; then
			# shellcheck disable=SC2086
			env $env_vars --load "$load_path" --background "$consumerd"
		else
			# shellcheck disable=SC2086
			env $env_vars --background "$consumerd"
		fi
		#$DIR/../src/bin/lttng-sessiond/$SESSIOND_BIN --background --consumerd32-path="$DIR/../src/bin/lttng-consumerd/lttng-consumerd" --consumerd64-path="$DIR/../src/bin/lttng-consumerd/lttng-consumerd" --verbose-consumer >>/tmp/sessiond.log 2>&1
		status=$?
		if [ "$withtap" -eq "1" ]; then
			ok $status "Start session daemon"
		fi
	fi
}

function start_lttng_sessiond()
{
	start_lttng_sessiond_opt 1 "$@"
}

function start_lttng_sessiond_notap()
{
	start_lttng_sessiond_opt 0 "$@"
}

function stop_lttng_sessiond_opt()
{
	local withtap=$1
	local signal=$2

	if [ -z "$signal" ]; then
		signal=SIGTERM
	fi

	local timeout_s=$3
	local dtimeleft_s=

	# Multiply time by 2 to simplify integer arithmetic
	if [ -n "$timeout_s" ]; then
		dtimeleft_s=$((timeout_s * 2))
	fi

	if [ -n "$TEST_NO_SESSIOND" ] && [ "$TEST_NO_SESSIOND" == "1" ]; then
		# Env variable requested no session daemon
		return 0
	fi

	local retval=0

	local runas_pids=
	runas_pids=$(pgrep "$RUNAS_MATCH")

	local pids=
	pids=$(pgrep "$SESSIOND_MATCH")

	if [ -n "$runas_pids" ]; then
		pids="$pids $runas_pids"
	fi

	if [ -z "$pids" ]; then
		if [ "$withtap" -eq "1" ]; then
			pass "No session daemon to kill"
		fi
		return 0
	fi

	diag "Killing (signal $signal) $SESSIOND_BIN and lt-$SESSIOND_BIN pids: $(echo "$pids" | tr '\n' ' ')"

	# shellcheck disable=SC2086
	if ! kill -s $signal $pids 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST; then
		retval=1
		if [ "$withtap" -eq "1" ]; then
			fail "Kill sessions daemon"
		fi
	else
		out=1
		while [ -n "$out" ]; do
			out=$(pgrep "${SESSIOND_MATCH}")
			if [ -n "$dtimeleft_s" ]; then
				if [ $dtimeleft_s -lt 0 ]; then
					out=
					retval=1
				fi
				dtimeleft_s=$((dtimeleft_s - 1))
			fi
			sleep 0.5
		done
		out=1
		while [ -n "$out" ]; do
			out=$(pgrep "$CONSUMERD_MATCH")
			if [ -n "$dtimeleft_s" ]; then
				if [ $dtimeleft_s -lt 0 ]; then
					out=
					retval=1
				fi
				dtimeleft_s=$((dtimeleft_s - 1))
			fi
			sleep 0.5
		done

		if [ "$withtap" -eq "1" ]; then
			if [ "$retval" -eq "0" ]; then
				pass "Wait after kill session daemon"
			else
				fail "Wait after kill session daemon"
			fi
		fi
	fi
	if [ "$signal" = "SIGKILL" ]; then
		if [ "$(id -u)" -eq "0" ]; then
			local modules=
			modules="$(lsmod | grep ^lttng | awk '{print $1}')"

			if [ -n "$modules" ]; then
				diag "Unloading all LTTng modules"
				modprobe -r "$modules"
			fi
		fi
	fi

	return $retval
}

function stop_lttng_sessiond()
{
	stop_lttng_sessiond_opt 1 "$@"
}

function stop_lttng_sessiond_notap()
{
	stop_lttng_sessiond_opt 0 "$@"
}

function sigstop_lttng_sessiond_opt()
{
	local withtap=$1
	local signal=SIGSTOP

	if [ -n "$TEST_NO_SESSIOND" ] && [ "$TEST_NO_SESSIOND" == "1" ]; then
		# Env variable requested no session daemon
		return
	fi

	PID_SESSIOND="$(pgrep "${SESSIOND_MATCH}") $(pgrep "$RUNAS_MATCH")"

	if [ "$withtap" -eq "1" ]; then
		diag "Sending SIGSTOP to lt-$SESSIOND_BIN and $SESSIOND_BIN pids: $(echo "$PID_SESSIOND" | tr '\n' ' ')"
	fi

	# shellcheck disable=SC2086
	if ! kill -s $signal $PID_SESSIOND 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST; then
		if [ "$withtap" -eq "1" ]; then
			fail "Sending SIGSTOP to session daemon"
		fi
	else
		out=1
		while [ $out -ne 0 ]; do
			pid="$(pgrep "$SESSIOND_MATCH")"

			# Wait until state becomes stopped for session
			# daemon(s).
			out=0
			for sessiond_pid in $pid; do
				state="$(ps -p "$sessiond_pid" -o state= )"
				if [[ -n "$state" && "$state" != "T" ]]; then
					out=1
				fi
			done
			sleep 0.5
		done
		if [ "$withtap" -eq "1" ]; then
			pass "Sending SIGSTOP to session daemon"
		fi
	fi
}

function sigstop_lttng_sessiond()
{
	sigstop_lttng_sessiond_opt 1 "$@"
}

function sigstop_lttng_sessiond_notap()
{
	sigstop_lttng_sessiond_opt 0 "$@"
}

function stop_lttng_consumerd_opt()
{
	local withtap=$1
	local signal=$2

	if [ -z "$signal" ]; then
		signal=SIGTERM
	fi

	local timeout_s=$3
	local dtimeleft_s=

	# Multiply time by 2 to simplify integer arithmetic
	if [ -n "$timeout_s" ]; then
		dtimeleft_s=$((timeout_s * 2))
	fi

	local retval=0

	PID_CONSUMERD="$(pgrep "$CONSUMERD_MATCH")"

	if [ -z "$PID_CONSUMERD" ]; then
		if [ "$withtap" -eq "1" ]; then
			pass "No consumer daemon to kill"
		fi
		return 0
	fi

	diag "Killing (signal $signal) $CONSUMERD_BIN pids: $(echo "$PID_CONSUMERD" | tr '\n' ' ')"

	# shellcheck disable=SC2086
	if ! kill -s $signal $PID_CONSUMERD 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST; then
		retval=1
		if [ "$withtap" -eq "1" ]; then
			fail "Kill consumer daemon"
		fi
	else
		out=1
		while [ $out -ne 0 ]; do
			pid="$(pgrep "$CONSUMERD_MATCH")"

			# If consumerds are still present check their status.
			# A zombie status qualifies the consumerd as *killed*
			out=0
			for consumer_pid in $pid; do
				state="$(ps -p "$consumer_pid" -o state= )"
				if [[ -n "$state" && "$state" != "Z" ]]; then
					out=1
				fi
			done
			if [ -n "$dtimeleft_s" ]; then
				if [ $dtimeleft_s -lt 0 ]; then
					out=0
					retval=1
				fi
				dtimeleft_s=$((dtimeleft_s - 1))
			fi
			sleep 0.5
		done
		if [ "$withtap" -eq "1" ]; then
			if [ "$retval" -eq "0" ]; then
				pass "Wait after kill consumer daemon"
			else
				fail "Wait after kill consumer daemon"
			fi
		fi
	fi

	return $retval
}

function stop_lttng_consumerd()
{
	stop_lttng_consumerd_opt 1 "$@"
}

function stop_lttng_consumerd_notap()
{
	stop_lttng_consumerd_opt 0 "$@"
}

function sigstop_lttng_consumerd_opt()
{
	local withtap=$1
	local signal=SIGSTOP

	PID_CONSUMERD="$(pgrep "$CONSUMERD_MATCH")"

	diag "Sending SIGSTOP to $CONSUMERD_BIN pids: $(echo "$PID_CONSUMERD" | tr '\n' ' ')"

	# shellcheck disable=SC2086
	kill -s $signal $PID_CONSUMERD 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	retval=$?

	if [ $retval -eq 1 ]; then
		if [ "$withtap" -eq "1" ]; then
			fail "Sending SIGSTOP to consumer daemon"
		fi
		return 1
	else
		out=1
		while [ $out -ne 0 ]; do
			pid="$(pgrep "$CONSUMERD_MATCH")"

			# Wait until state becomes stopped for all
			# consumers.
			out=0
			for consumer_pid in $pid; do
				state="$(ps -p "$consumer_pid" -o state= )"
				if [[ -n "$state" && "$state" != "T" ]]; then
					out=1
				fi
			done
			sleep 0.5
		done
		if [ "$withtap" -eq "1" ]; then
			pass "Sending SIGSTOP to consumer daemon"
		fi
	fi
	return $retval
}

function sigstop_lttng_consumerd()
{
	sigstop_lttng_consumerd_opt 1 "$@"
}

function sigstop_lttng_consumerd_notap()
{
	sigstop_lttng_consumerd_opt 0 "$@"
}

function list_lttng_with_opts ()
{
	local opts=$1
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN list $opts 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Lttng-tool list command with option $opts"
}

function create_lttng_session_no_output ()
{
	local sess_name=$1
	local opts="${@:2}"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN create $sess_name --no-output $opts 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Create session $sess_name in no-output mode"
}

function create_lttng_session_uri () {
	local sess_name=$1
	local uri=$2
	local opts="${@:3}"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN create $sess_name -U $uri $opts 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Create session $sess_name with uri:$uri and opts: $opts"
}

function create_lttng_session ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3
	local trace_path=$4
	local opt=$5

	if [ -z "$trace_path" ]; then
		# Use lttng-sessiond default output.
		trace_path=""
	else
		trace_path="-o $trace_path"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN create "$sess_name" $trace_path $opt 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [ $expected_to_fail -eq "1" ]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $ret "Create session $sess_name in $trace_path failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Create session $sess_name in $trace_path"
		fi
	fi
	return $ret
}

function create_lttng_session_ok ()
{
	create_lttng_session 1 0 "$@"
}

function create_lttng_session_fail ()
{
	create_lttng_session 1 1 "$@"
}

function create_lttng_session_notap ()
{
	create_lttng_session 0 0 "$@"
}


function enable_ust_lttng_channel ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3
	local channel_name=$4
	local opt=$5

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -u $channel_name -s $sess_name $opt 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable channel $channel_name for session $sess_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Enable channel $channel_name for session $sess_name"
		fi
	fi
	return $ret
}

function enable_ust_lttng_channel_ok ()
{
	enable_ust_lttng_channel 1 0 "$@"
}

function enable_ust_lttng_channel_fail ()
{
	enable_ust_lttng_channel 1 1 "$@"
}

function enable_ust_lttng_channel_notap ()
{
	enable_ust_lttng_channel 0 0 "$@"
}

function disable_ust_lttng_channel()
{
	local sess_name=$1
	local channel_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-channel -u $channel_name -s $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Disable channel $channel_name for session $sess_name"
}

function enable_lttng_mmap_overwrite_kernel_channel()
{
	local sess_name=$1
	local channel_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -s $sess_name $channel_name -k --output mmap --overwrite 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable channel $channel_name for session $sess_name"
}

function enable_lttng_mmap_discard_small_kernel_channel()
{
	local sess_name=$1
	local channel_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -s $sess_name $channel_name -k --output mmap --discard --subbuf-size=$(getconf PAGE_SIZE) --num-subbuf=2 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable small discard channel $channel_name for session $sess_name"
}

function enable_lttng_mmap_overwrite_small_kernel_channel()
{
	local sess_name=$1
	local channel_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -s $sess_name $channel_name -k --output mmap --overwrite --subbuf-size=$(getconf PAGE_SIZE) --num-subbuf=2 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable small discard channel $channel_name for session $sess_name"
}

function enable_lttng_mmap_overwrite_ust_channel()
{
	local sess_name=$1
	local channel_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-channel -s $sess_name $channel_name -u --output mmap --overwrite 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable channel $channel_name for session $sess_name"
}

function enable_ust_lttng_event ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3
	local event_name="$4"
	local channel_name=$5

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" $chan -s $sess_name -u 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ret=$?
		if [[ $withtap -eq "1" ]]; then
			ok $ret "Enable ust event $event_name for session $session_name failed as expected"
		fi
	else
		if [[ $withtap -eq "1" ]]; then
			ok $ret "Enable ust event $event_name for session $sess_name"
		fi
	fi
	return $ret
}

function enable_ust_lttng_event_ok ()
{
	enable_ust_lttng_event 1 0 "$@"
}

function enable_ust_lttng_event_fail ()
{
	enable_ust_lttng_event 1 1 "$@"
}

function enable_ust_lttng_event_notap ()
{
	enable_ust_lttng_event 0 0 "$@"
}

function enable_jul_lttng_event()
{
	sess_name=$1
	event_name="$2"
	channel_name=$3

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" $chan -s $sess_name -j 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable JUL event $event_name for session $sess_name"
}

function enable_jul_lttng_event_loglevel()
{
	local sess_name=$1
	local event_name="$2"
	local loglevel=$3
	local channel_name=$4

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event --loglevel $loglevel "$event_name" $chan -s $sess_name -j 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable JUL event $event_name for session $sess_name with loglevel $loglevel"
}

function enable_log4j_lttng_event()
{
	sess_name=$1
	event_name="$2"
	channel_name=$3

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" $chan -s $sess_name -l 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable LOG4J event $event_name for session $sess_name"
}

function enable_log4j_lttng_event_loglevel()
{
	local sess_name=$1
	local event_name="$2"
	local loglevel=$3
	local channel_name=$4

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event --loglevel $loglevel "$event_name" $chan -s $sess_name -l 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable LOG4J event $event_name for session $sess_name with loglevel $loglevel"
}

function enable_python_lttng_event()
{
	sess_name=$1
	event_name="$2"
	channel_name=$3

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" $chan -s $sess_name -p 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable Python event $event_name for session $sess_name"
}

function enable_python_lttng_event_loglevel()
{
	local sess_name=$1
	local event_name="$2"
	local loglevel=$3
	local channel_name=$4

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event --loglevel $loglevel "$event_name" $chan -s $sess_name -p 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable Python event $event_name for session $sess_name with loglevel $loglevel"
}

function enable_ust_lttng_event_filter()
{
	local sess_name="$1"
	local event_name="$2"
	local filter="$3"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" -s $sess_name -u --filter "$filter" 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable event $event_name with filtering for session $sess_name"
}

function enable_ust_lttng_event_loglevel()
{
	local sess_name="$1"
	local event_name="$2"
	local loglevel="$3"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" -s $sess_name -u --loglevel $loglevel 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable event $event_name with loglevel $loglevel"
}

function enable_ust_lttng_event_loglevel_only()
{
	local sess_name="$1"
	local event_name="$2"
	local loglevel="$3"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-event "$event_name" -s $sess_name -u --loglevel-only $loglevel 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Enable event $event_name with loglevel-only $loglevel"
}

function disable_ust_lttng_event ()
{
	local sess_name="$1"
	local event_name="$2"
	local channel_name="$3"

	if [ -z $channel_name ]; then
		# default channel if none specified
		chan=""
	else
		chan="-c $channel_name"
	fi

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-event "$event_name" -s $sess_name $chan -u 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Disable event $event_name for session $sess_name"
}

function disable_jul_lttng_event ()
{
	local sess_name="$1"
	local event_name="$2"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-event "$event_name" -s $sess_name -j >/dev/null 2>&1
	ok $? "Disable JUL event $event_name for session $sess_name"
}

function disable_log4j_lttng_event ()
{
	local sess_name="$1"
	local event_name="$2"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-event "$event_name" -s $sess_name -l >/dev/null 2>&1
	ok $? "Disable LOG4J event $event_name for session $sess_name"
}

function disable_python_lttng_event ()
{
	local sess_name="$1"
	local event_name="$2"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN disable-event "$event_name" -s $sess_name -p 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Disable Python event $event_name for session $sess_name"
}

function start_lttng_tracing_opt ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN start $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $? "Start tracing for session $sess_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Start tracing for session $sess_name"
		fi
	fi
}

function start_lttng_tracing_ok ()
{
	start_lttng_tracing_opt 1 0 "$@"
}

function start_lttng_tracing_fail ()
{
	start_lttng_tracing_opt 1 1 "$@"
}

function start_lttng_tracing_notap ()
{
	start_lttng_tracing_opt 0 1 "$@"
}

function stop_lttng_tracing_opt ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN stop $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $? "Stop lttng tracing for session $sess_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Stop lttng tracing for session $sess_name"
		fi
	fi
}

function stop_lttng_tracing_ok ()
{
	stop_lttng_tracing_opt 1 0 "$@"
}

function stop_lttng_tracing_fail ()
{
	stop_lttng_tracing_opt 1 1 "$@"
}

function stop_lttng_tracing_notap ()
{
	stop_lttng_tracing_opt 0 0 "$@"
}

function destroy_lttng_session ()
{
	local withtap=$1
	local expected_to_fail=$2
	local sess_name=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN destroy $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ret=$?
		if [ $withtap -eq "1" ]; then
			ok $ret "Destroy session $sess_name failed as expected"
		fi
	else
		if [ $withtap -eq "1" ]; then
			ok $ret "Destroy session $sess_name"
		fi
	fi
}

function destroy_lttng_session_ok ()
{
	destroy_lttng_session 1 0 "$@"

}

function destroy_lttng_session_fail ()
{
	destroy_lttng_session 1 1 "$@"
}

function destroy_lttng_session_notap ()
{
	destroy_lttng_session 0 0 "$@"
}

function destroy_lttng_sessions ()
{
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN destroy --all 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Destroy all lttng sessions"
}

function lttng_snapshot_add_output ()
{
	local expected_to_fail=$1
	local sess_name=$2
	local trace_path=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN snapshot add-output -s $sess_name $trace_path 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq 1 ]]; then
		test "$ret" -ne "0"
		ok $? "Added snapshot output file://$trace_path failed as expected"
	else
		ok $ret "Added snapshot output file://$trace_path"
	fi
}

function lttng_snapshot_add_output_ok ()
{
	lttng_snapshot_add_output 0 "$@"
}

function lttng_snapshot_add_output_fail ()
{
	lttng_snapshot_add_output 1 "$@"
}

function lttng_snapshot_del_output ()
{
	local expected_to_fail=$1
	local sess_name=$2
	local id=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN snapshot del-output -s $sess_name $id 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Deleted snapshot output id $id failed as expected"
	else
		ok $ret "Deleted snapshot output id $id"
	fi
}

function lttng_snapshot_del_output_ok ()
{
	lttng_snapshot_del_output 0 "$@"
}

function lttng_snapshot_del_output_fail ()
{
	lttng_snapshot_del_output 1 "$@"
}

function lttng_snapshot_record ()
{
	local sess_name=$1
	local trace_path=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN snapshot record -s $sess_name $trace_path 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Snapshot recorded"
}

function lttng_snapshot_list ()
{
	local sess_name=$1
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN snapshot list-output -s $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Snapshot list"
}

function lttng_save()
{
	local sess_name=$1
	local opts=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN save $sess_name $opts 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Session saved"
}

function lttng_load()
{
	local expected_to_fail=$1
	local opts=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN load $opts 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Load command failed as expected with opts: $opts"
	else
		ok $ret "Load command with opts: $opts"
	fi
}

function lttng_load_ok()
{
	lttng_load 0 "$@"
}

function lttng_load_fail()
{
	lttng_load 1 "$@"
}

function lttng_track()
{
	local expected_to_fail=$1
	local opts=$2
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN track $opts >$OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Track command failed as expected with opts: $opts"
	else
		ok $ret "Track command with opts: $opts"
	fi
}

function lttng_track_ok()
{
	lttng_track 0 "$@"
}

function lttng_track_fail()
{
	lttng_track 1 "$@"
}

function lttng_untrack()
{
	local expected_to_fail=$1
	local opts=$2
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN untrack $opts >$OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Untrack command failed as expected with opts: $opts"
	else
		ok $ret "Untrack command with opts: $opts"
	fi
}

function lttng_untrack_ok()
{
	lttng_untrack 0 "$@"
}

function lttng_untrack_fail()
{
	lttng_untrack 1 "$@"
}

function lttng_track_pid_ok()
{
	PID=$1
	"$TESTDIR/../src/bin/lttng/$LTTNG_BIN" track --kernel --pid=$PID 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Lttng track pid on the kernel domain"
}

function lttng_untrack_kernel_all_ok()
{
	"$TESTDIR/../src/bin/lttng/$LTTNG_BIN" untrack --kernel --pid --all 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ok $? "Lttng untrack all pid on the kernel domain"
}

function lttng_add_context_list()
{
	$TESTDIR/../src/bin/lttng/$LTTNG_BIN add-context --list 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	ok $ret "Context listing"
}

function add_context_lttng()
{
	local expected_to_fail="$1"
	local domain="$2"
	local session_name="$3"
	local channel_name="$4"
	local type="$5"

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN add-context -s $session_name -c $channel_name -t $type $domain  1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test $ret -ne "0"
		ok $? "Add context command failed as expected for type: $type"
	else
		ok $ret "Add context command for type: $type"
	fi
}

function add_context_ust_ok()
{
	add_context_lttng 0 -u "$@"
}

function add_context_ust_fail()
{
	add_context_lttng 1 -u "$@"
}

function add_context_kernel_ok()
{
	add_context_lttng 0 -k "$@"
}

function add_context_kernel_fail()
{
	add_context_lttng 1 -k "$@"
}

function validate_metadata_event ()
{
	local event_name=$1
	local nr_event_id=$2
	local trace_path=$3

	local metadata_file=$(find $trace_path | grep metadata)
	local metadata_path=$(dirname $metadata_file)

	which $BABELTRACE_BIN >/dev/null
	skip $? -ne 0 "Babeltrace binary not found. Skipping trace matches"

	local count=$($BABELTRACE_BIN --output-format=ctf-metadata $metadata_path | grep $event_name | wc -l)

	if [ "$count" -ne "$nr_event_id" ]; then
		fail "Metadata match with the metadata of $count event(s) named $event_name"
		diag "$count matching event id found in metadata"
	else
		pass "Metadata match with the metadata of $count event(s) named $event_name"
	fi

}

function trace_matches ()
{
	local event_name=$1
	local nr_iter=$2
	local trace_path=$3

	which $BABELTRACE_BIN >/dev/null
	skip $? -ne 0 "Babeltrace binary not found. Skipping trace matches"

	local count=$($BABELTRACE_BIN $trace_path | grep $event_name | wc -l)

	if [ "$count" -ne "$nr_iter" ]; then
		fail "Trace match"
		diag "$count matching events found in trace"
	else
		pass "Trace match"
	fi
}

function trace_match_only()
{
	local event_name=$1
	local nr_iter=$2
	local trace_path=$3

	which $BABELTRACE_BIN >/dev/null
	skip $? -ne 0 "Babeltrace binary not found. Skipping trace matches"

	local count=$($BABELTRACE_BIN $trace_path | grep $event_name | wc -l)
	local total=$($BABELTRACE_BIN $trace_path | wc -l)

	if [ "$nr_iter" -eq "$count" ] && [ "$total" -eq "$nr_iter" ]; then
		pass "Trace match with $total event $event_name"
	else
		fail "Trace match"
		diag "$total event(s) found, expecting $nr_iter of event $event_name and only found $count"
	fi
}

function validate_trace
{
	local event_name=$1
	local trace_path=$2

	which $BABELTRACE_BIN >/dev/null
	if [ $? -ne 0 ]; then
	    skip 0 "Babeltrace binary not found. Skipping trace validation"
	fi

	OLDIFS=$IFS
	IFS=","
	for i in $event_name; do
		traced=$($BABELTRACE_BIN $trace_path 2>/dev/null | grep $i | wc -l)
		if [ "$traced" -ne 0 ]; then
			pass "Validate trace for event $i, $traced events"
		else
			fail "Validate trace for event $i"
			diag "Found $traced occurences of $i"
		fi
	done
	ret=$?
	IFS=$OLDIFS
	return $ret
}

function validate_trace_count
{
	local event_name=$1
	local trace_path=$2
	local expected_count=$3

	which $BABELTRACE_BIN >/dev/null
	if [ $? -ne 0 ]; then
	    skip 0 "Babeltrace binary not found. Skipping trace validation"
	fi

	cnt=0
	OLDIFS=$IFS
	IFS=","
	for i in $event_name; do
		traced=$($BABELTRACE_BIN $trace_path 2>/dev/null | grep $i | wc -l)
		if [ "$traced" -ne 0 ]; then
			pass "Validate trace for event $i, $traced events"
		else
			fail "Validate trace for event $i"
			diag "Found $traced occurences of $i"
		fi
		cnt=$(($cnt + $traced))
	done
	IFS=$OLDIFS
	test $cnt -eq $expected_count
	ok $? "Read a total of $cnt events, expected $expected_count"
}

function trace_first_line
{
	local trace_path=$1

	which $BABELTRACE_BIN >/dev/null
	if [ $? -ne 0 ]; then
	    skip 0 "Babeltrace binary not found. Skipping trace validation"
	fi

	$BABELTRACE_BIN $trace_path 2>/dev/null | head -n 1
}

function validate_trace_exp()
{
	local event_exp=$1
	local trace_path=$2

	which $BABELTRACE_BIN >/dev/null
	skip $? -ne 0 "Babeltrace binary not found. Skipping trace validation"

	traced=$($BABELTRACE_BIN $trace_path 2>/dev/null | grep --extended-regexp ${event_exp} | wc -l)
	if [ "$traced" -ne 0 ]; then
		pass "Validate trace for expression '${event_exp}', $traced events"
	else
		fail "Validate trace for expression '${event_exp}'"
		diag "Found $traced occurences of '${event_exp}'"
	fi
	ret=$?
	return $ret
}

function validate_trace_only_exp()
{
	local event_exp=$1
	local trace_path=$2

	which $BABELTRACE_BIN >/dev/null
	skip $? -ne 0 "Babeltrace binary not found. Skipping trace matches"

	local count=$($BABELTRACE_BIN $trace_path | grep --extended-regexp ${event_exp} | wc -l)
	local total=$($BABELTRACE_BIN $trace_path | wc -l)

	if [ "$count" -ne 0 ] && [ "$total" -eq "$count" ]; then
		pass "Trace match with $total for expression '${event_exp}'"
	else
		fail "Trace match"
		diag "$total syscall event(s) found, only syscalls matching expression '${event_exp}' ($count occurrences) are expected"
	fi
	ret=$?
	return $ret
}

function validate_trace_empty()
{
	local trace_path=$1

	which $BABELTRACE_BIN >/dev/null
	if [ $? -ne 0 ]; then
	    skip 0 "Babeltrace binary not found. Skipping trace validation"
	fi

	events=$($BABELTRACE_BIN $trace_path 2>/dev/null)
	ret=$?
	if [ $ret -ne 0 ]; then
		fail "Failed to parse trace"
		return $ret
	fi

	traced=$(echo -n "$events" | wc -l)
	if [ "$traced" -eq 0 ]; then
		pass "Validate empty trace"
	else
		fail "Validate empty trace"
		diag "Found $traced events in trace"
	fi
	ret=$?
	return $ret
}

function regenerate_metadata ()
{
	local expected_to_fail=$1
	local sess_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN regenerate metadata -s $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Expected fail on regenerate metadata $sess_name"
	else
		ok $ret "Metadata regenerate $sess_name"
	fi
}

function regenerate_metadata_ok ()
{
	regenerate_metadata 0 "$@"
}

function regenerate_metadata_fail ()
{
	regenerate_metadata 1 "$@"
}

function regenerate_statedump ()
{
	local expected_to_fail=$1
	local sess_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN regenerate statedump -s $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Expected fail on regenerate statedump $sess_name"
	else
		ok $ret "Statedump regenerate $sess_name"
	fi
}

function regenerate_statedump_ok ()
{
	regenerate_statedump 0 "$@"
}

function regenerate_statedump_fail ()
{
	regenerate_statedump 1 "$@"
}

function rotate_session ()
{
	local expected_to_fail=$1
	local sess_name=$2

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN rotate $sess_name 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Expected fail on rotate session $sess_name"
	else
		ok $ret "Rotate session $sess_name"
	fi
}

function rotate_session_ok ()
{
	rotate_session 0 "$@"
}

function rotate_session_fail ()
{
	rotate_session 1 "$@"
}

function destructive_tests_enabled ()
{
	if [ ${LTTNG_ENABLE_DESTRUCTIVE_TESTS} = "will-break-my-system" ]; then
		return 0
	else
		return 1
	fi
}

function lttng_enable_rotation_timer ()
{
	local expected_to_fail=$1
	local sess_name=$2
	local period=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-rotation -s $sess_name --timer $period 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Expected fail when setting periodic rotation ($period) of session $sess_name"
	else
		ok $ret "Set periodic rotation ($period) of session $sess_name"
	fi
}

function lttng_enable_rotation_timer_ok ()
{
	lttng_enable_rotation_timer 0 $@
}

function lttng_enable_rotation_timer_fail ()
{
	lttng_enable_rotation_timer 1 $@
}

function lttng_enable_rotation_size ()
{
	local expected_to_fail=$1
	local sess_name=$2
	local size=$3

	$TESTDIR/../src/bin/lttng/$LTTNG_BIN enable-rotation -s $sess_name --size $size 1> $OUTPUT_DEST 2> $ERROR_OUTPUT_DEST
	ret=$?
	if [[ $expected_to_fail -eq "1" ]]; then
		test "$ret" -ne "0"
		ok $? "Expected fail on rotate session $sess_name"
	else
		ok $ret "Rotate session $sess_name"
	fi
}

function lttng_enable_rotation_size_ok ()
{
	lttng_enable_rotation_size 0 $@
}

function lttng_enable_rotation_size_fail ()
{
	lttng_enable_rotation_size 1 $@
}
