sudo ip link add link eth0 name eth0.11 type vlan id 1 egress 0:2 1:2 2:2 3:2 4:2 5:2 6:2 7:2
sudo ip link add link eth0 name eth0.12 type vlan id 2 egress 0:4 1:4 2:4 3:4 4:4 5:4 6:4 7:4
sudo systemctl daemon-reload
sudo systemctl restart ptp4l