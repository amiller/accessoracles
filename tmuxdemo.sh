#!/bin/bash

# Prepare
gcc -o cprograms/gpt1 cprograms/gpt1.c
gcc -o swapout swapout.c

# Create a new tmux session
tmux new-session -d -s demo

# Split the window into 4 panes
tmux split-window -h
tmux split-window -v
tmux select-pane -t 0
tmux split-window -v

# Run commands or scripts in each pane
tmux send-keys -t demo.0 'bash -c cprograms/gpt1 & echo $! > mypid; fg' Enter
tmux send-keys -t demo.1 'sleep 1; watch -n1 python3 pagemaps.py $(cat mypid)' Enter
tmux send-keys -t demo.2 'sleep 1; OUTPUT=1 watch -n1 python3 pagemaps.py $(cat mypid)' Enter
tmux send-keys -t demo.3 'sudo ./swapout $(cat mypid)' Enter

# Attach to the tmux session
tmux attach-session -t demo
