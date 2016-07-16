#!/bin/sh


configure_wifi() {
	echo -e '\n### Configure wifi =========================================\n' \
	&& uci set 'network.lan.ipaddr=192.168.4.1' \
	&& uci set 'wireless.radio0.disabled=0' \
	&& uci set 'wireless.radio0.channel=8' \
	&& uci set 'wireless.@wifi-iface[0]=wifi-iface' \
	&& uci set 'wireless.@wifi-iface[0].device=radio0' \
	&& uci set 'wireless.@wifi-iface[0].network=lan' \
	&& uci set 'wireless.@wifi-iface[0].mode=ap' \
	&& uci set 'wireless.@wifi-iface[0].encryption=psk2' \
	&& uci set 'wireless.@wifi-iface[0].ssid=LIGHT' \
	&& uci set 'wireless.@wifi-iface[0].key=ABCDEF12' \
	&& uci commit wireless ; wifi \
	&& echo -e '\nDONE\n'
}

install_dependencies() {
	echo -e '\n### Checking dependencies =========================================\n'
	if [ ! -f /usr/bin/ncat ]
	then
		opkg update
		opkg install ncat
	fi
}


configure_ip() {
	echo -e '\n### Configuring IP =========================================\n'
	ifconfig br-lan:0 192.168.4.2 up
}

configure_startup()
{
	echo -e '\n### Setting up startup script =========================================\n'
	local f=/etc/init.d/light-pong
	echo '#!/bin/sh /etc/rc.common' > $f
	echo 'START=99' >>$f
	echo 'boot() { start; }' >>$f
	echo 'start() { /root/openwrt.sh; }' >>$f
	chmod a+x $f /root/openwrt.sh
	$f enable
	sync
}

start_broadcaster() {
	echo -e '\n### Starting broker =========================================\n'
	ncat --broker -knl 2048
}

install_dependencies
configure_wifi
configure_ip
configure_startup
start_broadcaster
