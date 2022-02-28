#include <queue.h>
#include "skel.h"

struct rtable_entry
{
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

struct arp_entry
{
	uint32_t ip;
	uint8_t mac[6];
} __attribute__((packed));

//Functie de parsare a tabelei de routare.
int read_rtable(struct rtable_entry **rtable, char *arg)
{
	FILE *file;
	file = fopen(arg, "r");
	if (file == NULL)
	{
		fprintf(stderr, "Eroare fisier rtable!\n");
		exit(-1);
	}

	int capacity = 10000;
	char prefix[100];
	char next_hop[100];
	char mask[100];
	char interface[10];
	char buff[100];

	int i = 0;
	while (fgets(buff, sizeof(buff), file))
	{
		if (i == capacity)
		{
			//Capacitatea initiala este de 10000 de linii, realocarea se face prin adugarea a "capacity" de linii.
			capacity = capacity + capacity;
			(*rtable) = realloc(*rtable, sizeof(struct rtable_entry) * capacity);
		}
		//Fiecare element este identificat in linia citita si pus in tabela.
		sscanf(buff, "%s %s %s %s", prefix, next_hop, mask, interface);

		(*rtable)[i].prefix = inet_addr(prefix);
		(*rtable)[i].next_hop = inet_addr(next_hop);
		(*rtable)[i].mask = inet_addr(mask);
		(*rtable)[i].interface = atoi(interface);
		i++;
	}

	//Functia returneaza numarul de linii citite.
	fclose(file);
	return i;
}

//Functie de comparare necesara sortarii qsort din main.
//Se compara in functie de prefix, iar daca ip-urile sunt egale, atunci se compara in functie de masca.
int comparator(const void *a, const void *b)
{
	if ((*(struct rtable_entry *)a).prefix - (*(struct rtable_entry *)b).prefix == 0)
	{
		return (*(struct rtable_entry *)a).mask - (*(struct rtable_entry *)b).mask;
	}
	return (*(struct rtable_entry *)a).prefix - (*(struct rtable_entry *)b).prefix;
}

//Cautare binara a ip-ului "ip" in tabela de routare.
//Functia returneaza indicele din tabela care reprezinta intrarea pentru urmatoarea cea mai buna ruta.
int get_best_route(uint32_t ip, int left, int right, struct rtable_entry **rtable, int lines)
{
	if (right >= left)
	{
		int middle = left + (right - left) / 2;
		int current = -1;

		if ((ip & (*rtable)[middle].mask) == (*rtable)[middle].prefix)
		{
			if (current == -1)
			{
				current = middle;
			}
			while ((middle >= 0) && (*rtable)[middle].prefix == ((*rtable)[middle].mask & ip) &&
				   middle + 1 < lines)
			{
				//In caz ca a gasit ip-ul dorit, incearca sa gaseasca si cea mai lunga masca.
				if (ntohl((*rtable)[middle].mask) > ntohl((*rtable)[current].mask))
				{
					current = middle;
				}
				middle++;
			}
			return current;
		}

		if ((*rtable)[middle].prefix > (ip & (*rtable)[middle].mask))
			return get_best_route(ip, left, middle - 1, rtable, lines);

		if ((*rtable)[middle].prefix < (ip & (*rtable)[middle].mask))
			return get_best_route(ip, middle + 1, right, rtable, lines);
	}

	//Daca gaseste ip-ul in tabela de routare atunci returneaza -1.
	return -1;
}

//Cauta in tabela arp intrarea corespunzatoare ip-ului "ip".
struct arp_entry *get_arp_entry(uint32_t ip, struct arp_entry *arp_table, int arp_table_lines)
{
	for (int i = 0; i < arp_table_lines; i++)
		if (arp_table[i].ip == ip)
			return &arp_table[i];
	return NULL;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	packet m;
	int rc;

	init(argc - 2, argv + 2);

	struct rtable_entry *rtable = malloc(sizeof(struct rtable_entry) * 10000);
	struct arp_entry *arp_table = malloc(sizeof(struct arp_entry) * 10000);

	//lines = numarul de linii din tabela de routare.
	int lines = read_rtable(&rtable, argv[1]);

	//Sortarea tabelei de routare folosind functia comparator().
	qsort(rtable, lines, sizeof(struct rtable_entry), comparator);

	//Initializarea cozii si a contorului de linii pentru tabela arp, respectiv a capacitatii.
	struct queue *queue_packet = queue_create();
	int arp_table_lines = 0;
	int arp_table_capacity = 10000;

	while (1)
	{
		rc = get_packet(&m);
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;

		//Primeste un packet ARP.
		if (ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP)
		{
			//Se parseaza tabla folosind functia ajutatoare parse_arp().
			struct arp_header *arp_hdr = parse_arp(m.payload);
			if (arp_hdr != NULL)
			{
				//Daca routerul a primit un packet REPLY cu MAC-ul cerut la REQUEST.
				if (arp_hdr->op == htons(ARPOP_REPLY))
				{
					//Se extrage din tabela ARP intrarea corepunzatoare adresei ip a routerului cu care se incearca comunicarea.
					//Daca nu se cunoaste MAC-ul acestui router, atunci se adauga o linie noua in tabela ARP.
					struct arp_entry *verify = get_arp_entry(arp_hdr->spa, arp_table, arp_table_lines);
					if (verify == NULL)
					{
						//In cazul in care se ajunge la capacitatea maxima a tabelei ARP se realoca tabela.
						if (arp_table_lines == arp_table_capacity)
						{
							arp_table_capacity = arp_table_capacity + arp_table_capacity;
							arp_table = realloc(arp_table, sizeof(struct arp_entry) * arp_table_capacity);
						}

						//Routerul cu care se incearca comunicarea are adresa ARP_HEADER->SPA, deoarece el este sender-ul acestui packet.
						//De asemenea, adresa MAC a routerului este ARP_HEADER_SHA, el fiind sender-ul.
						arp_table[arp_table_lines].ip = arp_hdr->spa;
						memcpy(arp_table[arp_table_lines].mac, arp_hdr->sha, 6);
						arp_table_lines++;
					}

					//Se extrag toate packetele din coada si se trimit.
					while (!queue_empty(queue_packet))
					{
						packet *aux = (packet *)queue_deq(queue_packet);
						struct ether_header *aux_eth_hdr = (struct ether_header *)aux->payload;
						get_interface_mac(aux->interface, aux_eth_hdr->ether_shost);

						//Se seteaza ca destinatie MAC-ul primit prin protocolul ARP.
						memcpy(aux_eth_hdr->ether_dhost, arp_hdr->sha, 6);
						send_packet(aux->interface, aux);
					}
					continue;
				}
				if (arp_hdr->op == htons(ARPOP_REQUEST))
				{
					//Se trimite un ARP REPLY la sender-ul acestui packet de tip ARP REQUEST.
					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
					get_interface_mac(m.interface, eth_hdr->ether_shost);
					send_arp(arp_hdr->spa, arp_hdr->tpa, eth_hdr, m.interface, htons(ARPOP_REPLY));
					continue;
				}
			}
		}
		
		//Primeste un packet IP.
		if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP)
		{
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
			struct icmphdr *icmp_hdr = parse_icmp(m.payload);

			//Daca functia de parsare nu intoarce NULL, atunci verific daca packetul este de tip ICMP_ECHO si il trimit.
			//De asemenea, se verifica daca destinatia ip a acestui packet este adresa ip a acestui router.
			if (icmp_hdr != NULL)
			{
				if ((icmp_hdr->type == ICMP_ECHO) && (ip_hdr->daddr == inet_addr(get_interface_ip(m.interface))))
				{
					send_icmp(ip_hdr->saddr, inet_addr(get_interface_ip(m.interface)), eth_hdr->ether_dhost, eth_hdr->ether_shost, 0, 0, m.interface, icmp_hdr->un.echo.id, icmp_hdr->un.echo.sequence);
					continue;
				}
			}

			//Verific checksum-ul.
			if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0)
			{
				continue;
			}

			//Verific TTL-ul packetului.
			if (ip_hdr->ttl <= 1)
			{
				//In acest caz, se trimite un mesaj de eroare : "TIME_EXCEEDED".
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, m.interface);
				continue;
			}

			//Se decrementeaza TTL-ul si se recalculeaza checksum-ul.
			ip_hdr->ttl--;
			ip_hdr->check = 0;
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

			//Se cauta in tabela de routare cea mai buna cale catre ip-ul destinatie.
			int index = get_best_route(ip_hdr->daddr, 0, lines - 1, &rtable, lines);

			//Daca nu s-a gasit nici o cale, atunci se trimite un mesaj ICMP de tipul "DESTINATION UNREACHABLE".
			if (index == -1)
			{
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_DEST_UNREACH, 0, m.interface);
				continue;
			}
			else
			{
				struct rtable_entry *best_route = &rtable[index];
				struct arp_entry *arp_entry = get_arp_entry(ip_hdr->daddr, arp_table, arp_table_lines);
				
				//Daca s-a gasit intrarea pentru urmatorul router din cale, 
				//atunci se verifica daca se cunoaste adresa sa MAC verificandu-se tabela ARP.
				if (arp_entry == NULL)
				{
					//In cazul in care nu se cunoaste adresa sa MAC, atunci se trimite un ARP REQUEST.
					//Se creeaza un nou packet.
					packet *retransmition = malloc(sizeof(packet));
					retransmition->len = m.len;
					retransmition->interface = best_route->interface;
					memcpy(retransmition->payload, m.payload, m.len);

					//Se trimite pe broadcast si se pune packetul in coada.
					hwaddr_aton("ff:ff:ff:ff:ff:ff", eth_hdr->ether_dhost);
					eth_hdr->ether_type = htons(ETHERTYPE_ARP);
					get_interface_mac(best_route->interface, eth_hdr->ether_shost);
					send_arp(best_route->next_hop, inet_addr(get_interface_ip(best_route->interface)), eth_hdr, best_route->interface, htons(ARPOP_REQUEST));
					queue_enq(queue_packet, retransmition);
					continue;
				}

				//In cazul in care se cunoaste si MAC destinatie atunci se trimite packetul la urmatorul pas.
				get_interface_mac(best_route->interface, eth_hdr->ether_shost);
				memcpy(eth_hdr->ether_dhost, arp_entry->mac, sizeof(arp_entry->mac));
				send_packet(best_route->interface, &m);
			}
		}
	}
}
