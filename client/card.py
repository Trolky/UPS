"""Class representing sole card."""


class Card:
    def __init__(self, rank, suit):
        """Represents a playing card with a rank and a suit.

        Attributes:
            rank: The rank of the card (e.g., '7', '8', 'K', 'A').
            suit: The suit of the card (e.g., 'Hearts', 'Diamonds', 'Clubs', 'Spades').
        """
        self.rank: str = rank
        self.suit: str = suit

    def __str__(self):
        """pretty print a card"""
        return self.rank + self.suit
