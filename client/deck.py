"""Class representing deck of card."""

import random
from card import Card


class Deck:
    def __init__(self):
        """Create a deck of playing cards.

        A standard deck consists of 32 cards with ranks 7 through Ace across four suits.
        """
        ranks: list[str] = ['7', '8', '9', '10', 'J', 'Q', 'K', 'A']
        suits: list[str] = ['♥', '♦', '♣', '♠']
        self.cards: list[Card] = [Card(rank, suit) for rank in ranks for suit in suits]
        self.discard_pile = []

    def shuffle(self):
        """Shuffle the deck."""
        random.shuffle(self.cards)

    def deal(self, num_cards: int) -> list[Card]:
        """Deal a number of cards from the deck.

        Args:
            num_cards: The number of cards to deal.

        Returns:
            list[Card]: A list of dealt cards.
        """
        dealt_cards = self.cards[:num_cards]
        self.cards = self.cards[num_cards:]
        return dealt_cards

    def draw_card(self) -> Card | None:
        """Draw a single card from the deck.

        Returns:
            Card | None: The drawn card, or None if the deck is empty.
        """
        if self.cards:
            return self.cards.pop(0)
        return None

    def add_to_discard(self, card: Card):
        """Add a card to the discard pile.

        Args:
            card: The card to add to the discard pile.
        """
        self.discard_pile.append(card)

    def get_top_discard(self) -> Card | None:
        """Get the top card from the discard pile.

        Returns:
            Card | None: The top card of the discard pile, or None if the pile is empty.
        """
        return self.discard_pile[-1] if self.discard_pile else None

    def __str__(self):
        """Return a string representation of the deck and discard pile.

        Returns:
            str: A string detailing the cards in the deck and the discard pile.
        """
        deck = [card.rank + " " + card.suit for card in self.cards]
        pile = [card.rank + " " + card.suit for card in self.discard_pile]
        return f"Deck: {deck}\nPile: {pile}"
