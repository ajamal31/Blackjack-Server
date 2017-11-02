/* Name: Arun Jamal
 * Description: Function declarations for the all the functions in server.c
 */

#include <stdint.h>

struct player {
        char username[13];
        char cards[21];
        uint32_t bet;
        uint32_t bank;
        int count;
};

struct black_jack {
        char op_code;
        uint32_t response_arg;
        uint16_t seq_num;
        uint32_t min_bet;
        char active_player;
        char dealer_cards[21];
        char * cards;
        struct player * players[7];
};

int get_socket();

void open_connection(int socketfd);

void print_game(struct black_jack);

char * make_deck(int deck_size);

void print_deck (char * deck);

char draw_card(char * deck);

int card_value(char card);
