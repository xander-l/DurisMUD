/*****************************************************************************
 * file: cardgames.c,                                          part of Duris *
 * Usage: This contains code for varoius cardgames within Duris              *
 * Inception: 5/21/2015                                                      *
 *****************************************************************************/

#include "cardgames.h"
#include "interp.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Card functions!
const char *Card::getSuit()
{
  static const char Suit[4][14] = {"&+LClubs&n", "&+rDiamonds&n", "&+rHearts&n", "&+LSpades&n"};
  return Suit[(value-1)/13];
}

const char *Card::getValue()
{
  static const char Number[13][3] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
  return Number[(value-1)%13];
}

char *Card::Display()
{
  static char buf[25];

  sprintf( buf, "&+C%s&n %s", getValue(), getSuit() );

  return buf;
}

// Display cards with spacing to make even columns
char *Card::Display2()
{
  static char buf[25];

  sprintf( buf, "&+C%2s&n %-14s", getValue(), getSuit() );

  return buf;
}

// Hand functions!
char *Hand::Display()
{
  static char buf[MAX_STRING_LENGTH];
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display() );
    strcat( buf, "  " );
    temp = temp->nextCard;
  }

  return buf;
}

// Deck functions!

// Create the deck
Deck::Deck()
{
  P_Card temp;
  cards = NULL;
  numCards = 0;

  // Values for the cards are 1 .. 52.
  for( int i = 52; i >= 1; i-- )
  {
    temp = new Card(i, cards);
    cards = temp;
    numCards++;
  }
}

void Deck::Shuffle( int numShuffles )
{
  int count;
  // Left and right hands to shuffle.
  P_Card left, right, temp;

  if( numCards < 2 )
  {
    debug( "Trying to shuffle a deck with (%d) less than 2 cards.  Absurd.", numCards );
    return;
  }

  while( numShuffles-- > 0 )
  {
    count = numCards / 2;

    left = cards;
    // Cut Deck in half
    while( count-- > 1 )
    {
      left = left->nextCard;
    }
    // Left now points to the card before the split.
    right = left->nextCard;
    // Actual cut.
    left->nextCard = NULL;
    // Left now points to the top of the left hand set of cards.
    left = cards;
    cards = NULL;

    // Shuffle it back together (we actually do this upside down, but *shrug*).
    for(count = 0; count < numCards; count++ )
    {
      // Percentage chance to pull from the left hand.
      if( get_property("cardgames.shuffle.percentage", 50) >= number(1, 100) )
      {
        // Pull from left hand if there is a card.
        if( left != NULL )
        {
          // Remove top card.
          temp = left;
          left = left->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
        else
        {
          // Remove top card.
          temp = right;
          right = right->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
      }
      else
      {
        if( right != NULL )
        {
          // Remove top card.
          temp = right;
          right = right->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
        else
        {
          // Remove top card.
          temp = left;
          left = left->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
      }
    }
    if( count != numCards || left != NULL || right != NULL )
    {
      debug( "Shuffle: We have a problem: count %d, numCards %d, left %d, right %d",
        count, numCards, left, right );
    }
  }
}

P_Card Deck::DealACard()
{
  P_Card retCard = cards;

  if( cards == NULL )
  {
    return NULL;
  }

  cards = cards->nextCard;
  retCard->nextCard = NULL;
  return retCard;
}

char *Deck::Display()
{
  static char buf[MAX_STRING_LENGTH];
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display() );
    strcat( buf, "  " );
    temp = temp->nextCard;
  }

  return buf;
}

char *Deck::Display2()
{
  static char buf[MAX_STRING_LENGTH];
  int count = 0;
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display2() );
    strcat( buf, " | " );
    temp = temp->nextCard;
    if( (++count)%5 == 0 )
    {
      strcat( buf, "\n\r" );
    }
  }

  return buf;
}

int cards_object( P_obj obj, P_char ch, int cmd, char *argument )
{
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  static P_Deck theDeck = new Deck();
  int num;

  if( cmd == CMD_SET_PERIODIC || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_SAY )
  {
    argument = one_argument(argument, arg);
    if( !strcmp(arg, "create") )
    {
      if( theDeck != NULL )
      {
        send_to_char( "Deleting old deck...\n\r", ch );
        delete theDeck;
      }
      theDeck = new Deck();
    }
    else if( !strcmp(arg, "shuffle") )
    {
      argument = one_argument(argument, arg);
      num = atoi( arg );
      // Shuffle at least one time.
      num = (num<1) ? 1 : num;
      while( num-- > 0 )
      {
        send_to_char( "Shuffling...  ", ch );
        theDeck->Shuffle( 1 );
      }
      send_to_char( "\n\r", ch );
    }
    else if( !strcmp(arg, "shuffle2") )
    {
      argument = one_argument(argument, arg);
      num = atoi( arg );
      // Shuffle at least one time.
      num = (num<1) ? 1 : num;
      sprintf( buf, "Shuffling %d times....   ", num );
      send_to_char( buf, ch );
      theDeck->Shuffle( num );
      send_to_char( "done.\n\r", ch );
    }
    else if( !strcmp( arg, "display" ) )
    {
      send_to_char( "Your Deck:\n\r", ch );
      send_to_char( theDeck->Display(), ch );
      send_to_char( ".\n\r", ch );
    }
    else if( !strcmp( arg, "display2" ) )
    {
      send_to_char( "Your Deck:\n\r", ch );
      send_to_char( theDeck->Display2(), ch );
      send_to_char( ".\n\r", ch );
    }
  }

  return FALSE;
}

