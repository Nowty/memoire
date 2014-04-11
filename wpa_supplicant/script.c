#include "wpa_ctrl/wpa_ctrl.h"
#include "script.h"
#include <pthread.h>


/*
 * TODO : Continuer le parsing de event 
 * TODO : Problème timing ?
 * TODO : Log ou txt ?
 * 
 */



/* 
 * Insert the logs into the logfile
 */
static void log_event(enum log_events log, const char *arg) {
	static int isOpen = 0;
	if(!isOpen) {
		openlog("wifi_script", LOG_PID, LOG_DAEMON);
		isOpen = 1;
	}
	switch(log) {
		case LOG_CUSTOM_INFO:
			syslog(LOG_NOTICE, "%s", arg);
			break;
		case LOG_BOOT:
			syslog(LOG_NOTICE, "<Boot> Connected to: %s", arg);
			break;
		case LOG_TRY_CONNECTION:
			syslog(LOG_NOTICE, "Trying: %s", arg);
			break;
		case LOG_CONNECTED:
			syslog(LOG_NOTICE, "Connected: %s", arg);
			break;
		case LOG_DISCONNECTED:
			syslog(LOG_NOTICE, "Disconnected from: %s", arg);
			break;
		case LOG_CONNECTION_LOST:
			syslog(LOG_NOTICE, "Connection Lost: %s", arg);
			break;
		case LOG_CONNECTION_REESTABLISHED:
			syslog(LOG_NOTICE, "Connection reestablished: %s", arg);
			break;
		case LOG_TERMINATE:
			syslog(LOG_NOTICE, "Aborted by user");
			break;
		case LOG_FAILED:
			syslog(LOG_NOTICE, "Interface connection failed");
			break;
		case LOG_ERROR:
			syslog(LOG_ERR, "Error: %s", arg);
			break;
	}
}

/*
 * Parse all the event received from wpa_supplicant and execute the resulting action
 */
static void parse_event(const char *reply) {
	//Removing priority level <.>
	const char *event, *addr;
	event = reply;
	if(*event == '<') {
		event = strchr(event, '>');
		if(event) {
			event++;
		}
		else {
			event = reply;
		}
	}

	if(match(event, WPA_EVENT_CONNECTED)) {
		char arg[BUF];
		sprintf(arg, "wpa_s time: %ldsec %.3ums, dhcp time: %ldsec %.3ums", addr, wpa_time.time, wpa_time.millitm, dhcp_time.time, dhcp_time.millitm);
		log_event(LOG_CONNECTED, arg);
	}
	
}

/*
 * Execute the function w.r.t the special action received
 */
static void execute_action(enum wpa_action action, const char * ssid) {
	switch(action) {
		case ACTION_BOOT_PROCESS:
			boot_process();
			break;
		case ACTION_CONNECT_STUDENT:
			connect_student();
			break;
		case ACTION_CONNECT_EDUROAM:
			connect_eduroam();
			break;
		case ACTION_CONNECT_UCLOUVAIN:
			connect_uclouvain();
			break;
		case ACTION_DISCONNECT:
			commands("DISCONNECT");
			commands("DISABLE_NETWORK 0");
			commands("DISABLE_NETWORK 1");
			commands("DISABLE_NETWORK 2");
			log_event(LOG_DISCONNECTED, ssid);
			break;
		case ACTION_CREATE_NETWORKS:
			create_networks();
			log_event(LOG_CUSTOM_INFO, "Networks created");
			break;
		case ACTION_MAKE_LOG:
			system("logread | grep wifi_script >> log");
			break;	
	}
}


/*
 * Execute and send the command requests to wpa_supplicant
 */
static void commands(char *cmd)
{
	char reply[BUF];
	size_t len;
	int ret;
	
	if(ctrl == NULL) {
		//log_event(LOG_ERROR, "Interface not connected to wpa_supplicant");
		exit(-1);
	}
	ret = wpa_ctrl_request(ctrl, cmd, os_strlen(cmd), reply, &len, NULL);
	if(ret < 0) {
		//log_event(LOG_ERROR, ("Command '%s' timed out or failed", cmd));
		exit(-1);
	}
}


/*
 * Starts wpa_supplicant with student.UCLouvain config file in
 * background and starts the DHCP negociation
 */
void boot_process() {
	system("killall hostapd");
	ftime(&wpa_start);
	system("wpa_supplicant -B -D nl80211 -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant_maison.conf");
	ftime(&wpa_end);
	ftime(&dhcp_start);
	system("udhcpc -t 0 -i wlan0");
	ftime(&dhcp_end);
	timeDiff(wpa_start, wpa_end, &wpa_time);
	timeDiff(dhcp_start, dhcp_end, &dhcp_time);
	log_event(LOG_TRY_CONNECTION, "student.UCLouvain");
}


/*
 * Create the configuration for all the networks
 * ID=0: student.UCLOUVAIN
 * ID=1: eduroam
 * ID=2: UCLOUVAIN
 */
static void create_networks() {
	int i;
	for(i = 0; i < 2; i++) {
		commands("ADD_NETWORK");
	}
	config_network(1, "eduroam", "WPA-EAP", "PEAP", "CCMP", "ingi1@wifi.uclouvain.be", "OLIelmdrad99", "/etc/wpa_supplicant/chain-radius.pem", "peaplabel=0", "auth=MSCHAPV2", NULL);

	config_network(2, "UCLouvain", "WPA-EAP", "TTLS", NULL, "ingi1@wifi.uclouvain.be", "OLIelmdrad99", "/etc/wpa_supplicant/chain-radius.pem", NULL, "auth=PAP", NULL);	
}


/*
* Configure the newly created networks
*/
 void config_network(int network, char *ssid, char *key_mgmt, char *eap, char *pairwise, char *identity, char *password, char *ca_cert, char *phase1, char *phase2) {
	char cmd[1024];

	os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d ssid \"%s\"", network,ssid);
	commands(cmd);

	if(key_mgmt != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d key_mgmt %s", network, key_mgmt);
		commands(cmd);
	}

	if(eap != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d eap %s", network, eap);
		commands(cmd);
	}

	if(pairwise != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d pairwise %s", network, pairwise);
		commands(cmd);
	}

	if(identity != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d identity \"%s\"", network, identity);
		commands(cmd);
	}

	if(password != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d password \"%s\"", network, password);
		commands(cmd);
	}

	if(ca_cert != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d ca_cert \"%s\"", network, ca_cert);
		commands(cmd);
	}

	if(phase1 != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d phase1 \"%s\"", network, phase1);
		commands(cmd);
	}

	if(phase2 != NULL) {
		os_snprintf(cmd, sizeof(cmd), "SET_NETWORK %d phase2 \"%s\"",network, phase2);
		commands(cmd);
	}
}

/*
 * Connects wpa_supplicant to the eduroam network
 */
static void connect_eduroam() {
	commands("ENABLE_NETWORK 1");
	ftime(&wpa_start);
	commands("SELECT_NETWORK 1");
	ftime(&wpa_end);
	system("killall udhcpc");
	ftime(&dhcp_start);
	system("udhcpc -i wlan0");
	ftime(&dhcp_end);
	timeDiff(wpa_start, wpa_end, &wpa_time);
	timeDiff(dhcp_start, dhcp_end, &dhcp_time);
	log_event(LOG_TRY_CONNECTION, "eduroam");
}

/*
 * Connects wpa_supplicant to the UCLouvain network
 */
static void connect_uclouvain() {
	commands("ENABLE_NETWORK 2");
	ftime(&wpa_start);
	commands("SELECT_NETWORK 2");
	ftime(&wpa_end);
	system("killall udhcpc");
	ftime(&dhcp_start);
	system("udhcpc -i wlan0");
	ftime(&dhcp_end);
	timeDiff(wpa_start, wpa_end, &wpa_time);
	timeDiff(dhcp_start, dhcp_end, &dhcp_time);
	log_event(LOG_TRY_CONNECTION, "UCLouvain");
}

/*
 * Connects wpa_supplicant to the student.UCLouvain network
 */
static void connect_student() {
	commands("ENABLE_NETWORK 0");
	ftime(&wpa_start);
	commands("SELECT_NETWORK 0");
	ftime(&wpa_end);
	system("killall udhcpc");
	ftime(&dhcp_start);
	system("udhcpc -i wlan0");
	ftime(&dhcp_end);
	timeDiff(wpa_start, wpa_end, &wpa_time);
	timeDiff(dhcp_start, dhcp_end, &dhcp_time);
	log_event(LOG_CONNECTED, "student.UCLouvain");
}

/*
 * Loop testing the connection of the different SSIDs
 */
void *connection_loop(void * p_data) {
	while(1) {
		sleep(10);
		execute_action(ACTION_DISCONNECT, "student.UCLouvain");
		sleep(5);
		execute_action(ACTION_CONNECT_EDUROAM, "");
		sleep(10);
		execute_action(ACTION_DISCONNECT, "eduroam");
		sleep(5);
		execute_action(ACTION_CONNECT_UCLOUVAIN, "");
		sleep(10);
		execute_action(ACTION_DISCONNECT, "UCLouvain");
		sleep(5);
		execute_action(ACTION_CONNECT_STUDENT, "");
	}
	return NULL;
}


void *wpa_loop(void *p_data) {
	char reply[BUF];
	size_t reply_len;
	//select()
	int ctrl_fd, r, err;
	fd_set ctrl_fds;
	struct timeval timeout;
	
	ctrl = wpa_ctrl_open(DEFAULT_CTRL_IFACE);
	if(ctrl == NULL) {
		log_event(LOG_ERROR, "Unable to open wpa_supplicant control interface");
		exit(-1);
	}
	err = wpa_ctrl_attach(ctrl);
	if(err == -1) {
		wpa_ctrl_close(ctrl);
		log_event(LOG_ERROR, "wpa_ctrl_attach error");
		exit(-1);
	}
	if(err == -2) {
		wpa_ctrl_close(ctrl);
		log_event(LOG_ERROR, "wpa_ctrl_attach timeout");
		exit(-1);
	}
	
	//Loop for incoming events from wpa_supplicant
	while(1) {
		FD_ZERO(&ctrl_fds);
		ctrl_fd = wpa_ctrl_get_fd(ctrl);
		FD_SET(ctrl_fd, &ctrl_fds);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		//Wait for event
		r = select(ctrl_fd+1, &ctrl_fds, NULL, &ctrl_fds, &timeout);
		switch(r) {
			case -1:
				log_event(LOG_ERROR, "Error during select()\n");
				break;
			case 0:
				if(wpa_ctrl_request(ctrl, "PING", strlen("PING"), reply, &reply_len, NULL))
					reply_len = 0;
				reply[reply_len] = '\0';
				if(match(reply, "PONG", strlen("PONG"))) {
					log_event(LOG_ERROR, "wpa_supplicant not responding\n");
				}
				break;
			default: //event from wpa_supplicant received
				reply_len = BUF-1;
				wpa_ctrl_recv(ctrl, reply, &reply_len);
				reply[reply_len] = '\0';
				parse_event(reply);
				break;
		}
	}
	wpa_ctrl_detach(ctrl);
	wpa_ctrl_close(ctrl);
	return NULL;
}



/*******************************
 * TESTING AREA
 *******************************/


void *connectMaison(void * p_data) {
	while(1) {
		printf("Before\n");
		sleep(10);
		printf("Disconnect\n");
		commands("DISCONNECT");
		printf("Before 3\n");
		sleep(3);
		printf("Reconnect\n");
		commands("SELECT_NETWORK 0");
	}
	return NULL;
}



int main(int argc, char ** argv) {
	log_event(LOG_CUSTOM_INFO, "Starting script");
	int r = 0;
	pthread_t wpa_thread, loop_thread;

	execute_action(ACTION_BOOT_PROCESS, "");

	printf("Creation du thread wpa_supplicant\n");
	r = pthread_create(&wpa_thread, NULL, connectMaison, NULL);
	if(!r) {
		sleep(2);
		printf("Creation du thread loop\n");
		r = pthread_create(&loop_thread, NULL, wpa_loop, NULL);
		if(r)
			fprintf(stderr, "%s", strerror(r));
	}
	else 
		fprintf(stderr, "%s", strerror(r));	

	pthread_join(wpa_thread, NULL);
	pthread_join(wpa_thread, NULL);

	closelog();

	return 0;
}
