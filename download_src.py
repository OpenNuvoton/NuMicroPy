from __future__ import annotations
import os
import sys
import git
import argparse
import subprocess
from rich import console, progress

MICROPYTHON_URL = 'https://github.com/micropython/micropython' 
MICROPYTHON_BRANCH = 'v1.24.0' 
MICROPYTHON_DIR = 'micropython'

M460BSP_URL = 'https://github.com/OpenNuvoton/m460bsp' 
M460BSP_BRANCH = 'master' 
M460BSP_DIR = 'M460BSP'

M46X_PATCH_DIR = 'M46x_patch'

M55M1BSP_URL = 'https://github.com/OpenNuvoton/M55M1BSP' 
M55M1BSP_BRANCH = 'master' 
M55M1BSP_DIR = 'M55M1BSP'

M55M1_PATCH_DIR = 'M55M1_patch'

M5531BSP_URL = 'https://github.com/OpenNuvoton/M5531BSP' 
M5531BSP_BRANCH = 'master' 
M5531BSP_DIR = 'M5531BSP'

M5531_PATCH_DIR = 'M5531_patch'

parser=argparse.ArgumentParser(description='Download numicropy source')
parser.add_argument("--board", help='NuMaker-M467HJ/NuMaker-M55M1')
args=parser.parse_args()

if args.board == 'NuMaker-M467HJ':
	SOC='M460'
elif args.board == 'NuMaker-M55M1':
	SOC='M55M1'	
elif args.board == 'NuMaker-M5531':
	SOC='M5531'	
else:
	print('Board not support')
	sys.exit()

class GitRemoteProgress(git.RemoteProgress):
    OP_CODES = [
        "BEGIN",
        "CHECKING_OUT",
        "COMPRESSING",
        "COUNTING",
        "END",
        "FINDING_SOURCES",
        "RECEIVING",
        "RESOLVING",
        "WRITING",
    ]
    OP_CODE_MAP = {
        getattr(git.RemoteProgress, _op_code): _op_code for _op_code in OP_CODES
    }

    def __init__(self) -> None:
        super().__init__()
        self.progressbar = progress.Progress(
            progress.SpinnerColumn(),
            # *progress.Progress.get_default_columns(),
            progress.TextColumn("[progress.description]{task.description}"),
            progress.BarColumn(),
            progress.TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
            "eta",
            progress.TimeRemainingColumn(),
            progress.TextColumn("{task.fields[message]}"),
            console=console.Console(),
            transient=False,
        )
        self.progressbar.start()
        self.active_task = None

    def __del__(self) -> None:
        # logger.info("Destroying bar...")
        self.progressbar.stop()

    @classmethod
    def get_curr_op(cls, op_code: int) -> str:
        """Get OP name from OP code."""
        # Remove BEGIN- and END-flag and get op name
        op_code_masked = op_code & cls.OP_MASK
        return cls.OP_CODE_MAP.get(op_code_masked, "?").title()

    def update(
        self,
        op_code: int,
        cur_count: str | float,
        max_count: str | float | None = None,
        message: str | None = "",
    ) -> None:
        # Start new bar on each BEGIN-flag
        if op_code & self.BEGIN:
            self.curr_op = self.get_curr_op(op_code)
            # logger.info("Next: %s", self.curr_op)
            self.active_task = self.progressbar.add_task(
                description=self.curr_op,
                total=max_count,
                message=message,
            )

        self.progressbar.update(
            task_id=self.active_task,
            completed=cur_count,
            message=message,
        )

        # End progress monitoring on each END-flag
        if op_code & self.END:
            # logger.info("Done: %s", self.curr_op)
            self.progressbar.update(
                task_id=self.active_task,
                message=f"[bright_black]{message}",
            )


isMicropython=os.path.isdir(MICROPYTHON_DIR)
if not isMicropython:
	print("Download micropython {}".format(MICROPYTHON_BRANCH))
	git.Repo.clone_from(MICROPYTHON_URL,MICROPYTHON_DIR, branch=MICROPYTHON_BRANCH, recursive=True,  progress=GitRemoteProgress())

if SOC == 'M460':
	isBSP=os.path.isdir(M460BSP_DIR)
	if not isBSP:
		print("Download BSP {}".format(SOC))
		git.Repo.clone_from(M460BSP_URL, M460BSP_DIR, branch=M460BSP_BRANCH, recursive=True,  progress=GitRemoteProgress())
elif SOC == 'M55M1':
	isBSP=os.path.isdir(M55M1BSP_DIR)
	if not isBSP:
		print("Download BSP {}".format(SOC))
		git.Repo.clone_from(M55M1BSP_URL, M55M1BSP_DIR, branch=M55M1BSP_BRANCH, recursive=True,  progress=GitRemoteProgress())
elif SOC == 'M5531':
	isBSP=os.path.isdir(M5531BSP_DIR)
	if not isBSP:
		print("Download BSP {}".format(SOC))
		git.Repo.clone_from(M5531BSP_URL, M5531BSP_DIR, branch=M5531BSP_BRANCH, recursive=True,  progress=GitRemoteProgress())
	
	
#build mpy-cross tool
print("Generate mpy-cross tool")
os.chdir('micropython/mpy-cross')
subprocess.run('make')
os.chdir('../../')

#patch source file
print("Patch source file")
if SOC == 'M460':
	os.chdir('patch/M46x')
	subprocess.run('./run_patch.sh')
	os.chdir('../../')
#elif SOC == 'M55M1':
#	os.chdir('patch/M55M1')
#	subprocess.run('./run_patch.sh')
#	os.chdir('../../')

