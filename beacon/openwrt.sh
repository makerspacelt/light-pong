#!/bin/sh


configure_wifi() {
	echo -e '\n### Configure wifi =========================================\n' \
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
	opkg update
	opkg install socat
}


configure_ip() {
	ifconfig br-lan:0 192.168.4.2 up
}

start_broadcaster() {
	socat # something
}

