"""
Copyright (c) 2022 Ettus Research, A National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

import functools
import re
import subprocess

SSH_COMMAND = 'ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 {user}@{host}'
SCP_COMMAND = 'scp -o StrictHostKeyChecking=no -o ConnectTimeout=10 {src} {user}@{host}:{dest}'

REBOOT_SCRIPT = r'''
max_timeout=600

function ssh_cmd() {{
  ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 {user}@{host} $1
}}

if ! {{ boot_id=$(ssh_cmd "cat /proc/sys/kernel/random/boot_id") \
    && [[ $boot_id ]]; }}; then
  echo "Unable to retrieve boot ID" >&2
  exit 1
fi

ssh_cmd "reboot"

timeout_at=$(( SECONDS + max_timeout ))
until new_boot_id=$(ssh_cmd "cat /proc/sys/kernel/random/boot_id") \
    && [[ $new_boot_id != "$boot_id" ]]; do
  if (( SECONDS > timeout_at )); then
    echo "System did not reboot within timeout" >&2
    exit 1
  fi
  sleep 5
done

echo "System successfully rebooted" >&2
'''

class BaseCommand:
    """
    Base class for all usrpctl commands

    Implements some helper useful for all command classes.
    """

    @classmethod
    def command_name(cls):
        """
        By default the command name is the class name (lowercase) without
        the trailing "Command"
        """
        return re.sub("(.*)Command$", r"\1", cls.__name__).lower()

    @classmethod
    def add_parser(cls, parser):
        """
        add_parser is used to tell usrpctl argparse which command line
        argument the command accepts.
        """

    def _to_arg_list(self, args):
        """
        Takes parsed arguments from argparse and converts it to
        a flat list of key value pairs where the key is prepended with
        two dashes. If value is falsy the pair is ommitted.
        This is suitable to pass parsed arguments into another process.
        Example: Suppose args is:
        Namespace(foo='fval', bar=2, baz=None)
        then this would result in this list:
        ['--foo', 'fval', '--bar', 2]
        """
        arg_list = {f"--{key}": value for key, value in vars(args).items() if value}
        return list(functools.reduce(
                lambda left, right: left + right, arg_list.items())) if arg_list else []

    def _external_process(self, command):
        """
        Run command in an external process.

        stderr is redirected into stdout and the oupput is
        yielded while the process is running.
        throws CalledProcessError on non zero returncode
        """
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True) as process:
            for line in iter(process.stdout.readline, ""):
                yield line.rstrip()
            returncode = process.wait()
            if returncode != 0:
                raise subprocess.CalledProcessError(returncode, command)

    def _reboot(self, user, host):
        cmd = REBOOT_SCRIPT.format(user=user, host=host)
        yield from self._external_process(["bash", "-c", cmd])

    def _remote_cmd(self, user, host, cmd):
        """
        Executes a command via SSH.
        """
        ssh_cmd = SSH_COMMAND.format(user=user, host=host)
        yield from self._external_process(ssh_cmd.split(' ') + [cmd])

    def _remote_copy(self, user, host, src, dest):
        """
        Uses scp to copy a file to a remote host
        """
        cp_cmd = SCP_COMMAND.format(user=user, host=host, src=src, dest=dest)
        yield from self._external_process(cp_cmd.split(' '))

    def _process_output(self, line):
        print(line)

    def _run_commands(self, usrp, args):
        print(f"{self.__class__.__name__} is not yet implemented."
              f"arguments usrp: {usrp} args: {args}")

    def is_multi_device_capable(self):
        """
        Tell whether this command can serve multiple USRPs at once
        """
        return False

    def run(self, usrps, args):
        """
        Run the command
        """
        for usrp in usrps:
            print(f'{self.command_name()} {usrp.to_string()}')
            for line in self._run_commands(usrp, args):
                self._process_output(line)
