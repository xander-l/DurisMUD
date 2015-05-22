#include "structs.h"
#include "prototypes.h"

typedef struct Deck *P_Deck;
typedef struct Card *P_Card;
typedef struct Hand *P_Hand;

class Card
{
  friend class Hand;
  friend class Deck;

  private:
    int value;

  protected:
    P_Card nextCard;

    const char *getSuit();
    const char *getValue();

  public:
    Card( int val, P_Card next = NULL ) {value = (val<1) ? 1 : (val>52) ? 52 : val; nextCard = next;};
    char *Display();
    char *Display2();

};

class Deck
{
  protected:
    P_Card cards;
    int numCards;

  public:
    Deck();
    void Shuffle( int numShuffles );
    P_Card DealACard();
    char *Display();
    char *Display2();
    ~Deck() { while( cards != NULL ) {P_Card a = cards; cards = cards->nextCard; delete a;} };
};

class Hand
{
  protected:
    P_Card cards;

  public:
    Hand() { cards = NULL; };
    void RecieveCard( P_Card newCard ) { newCard->nextCard = cards; cards = newCard; };
    char *Display();
    P_Card Fold() { P_Card a = cards; cards = NULL; return a; };
    ~Hand() { while( cards != NULL ) {P_Card a = cards; cards = cards->nextCard; delete a;} };
};
