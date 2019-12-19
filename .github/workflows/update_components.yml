name: Update Components

on:
  schedule: [push]
    #- cron: 32 20 * * *

jobs:
  check:
    runs-on: ubuntu-latest
    steps: 
    - name: check for micro-etvd update
      run: curl https://github.com/1000001101000/micro-evtd/raw/master/bins/micro-evtd-amd64 2>/dev/null | md5sum > /tmp/source 
    - name: check for micro-etvd update
      run: wget -o /tmp/micro-evtd-amd64 https://github.com/${{github.repository}}raw/master/micro-evtd 2>/dev/null; cat /tmp/micro-evtd-amd64 | md5sum > /tmp/dest
    - name: check for micro-etvd update
      run: diff /tmp/source /tmp/dest || echo "needs update"
    
