"""Class representing player."""
from card import Card


class Player:
    """
    Represents a player in the card game.

    Attributes:
        name: The name of the player.
        hand: The cards currently held by the player.
    """
    def __init__(self, name: str = None):
        """Initialize a player with a name and an empty hand.

        Args:
            name (str): The name of the player.
        """
        self.name: str = name
        self.hand: list[Card] = []

    def draw_card(self, card: Card):
        """Add a card to the player's hand.

        Args:
            card (Card): The card to be added to the player's hand.
        """
        if card:
            self.hand.append(card)

    def play_card(self, card: Card, top_discard: Card, allowed_suite: str) -> bool:
        """Try to play a card.

        Checks if the card can be played based on the game rules.

        Args:
            card: The card to be played.
            top_discard: The top card of the discard pile.
            allowed_suite: The currently allowed suit for play.

        Returns:
            bool: True if the card was successfully played, False otherwise.
        """
        if allowed_suite:
            if card.suit == allowed_suite:
                self.hand.remove(card)
                return True

        if card.suit == top_discard.suit or card.rank == top_discard.rank or card.rank == 'Q':
            self.hand.remove(card)
            return True
        return False

    def has_won(self) -> bool:
        """Check if the player has won by having an empty hand.

        Returns:
            bool: True if the player's hand is empty, indicating a win; False otherwise.
        """
        return len(self.hand) == 0

    def get_hand_size(self) -> int:
        """Return the number of cards in the player's hand.

        Returns:
            int: The number of cards currently held by the player.
        """
        return len(self.hand)

    def __str__(self):
        """String representation of the player's name and hand size.

        Returns:
            str: A formatted string showing the player's name and the number of cards in hand.
        """
        return f"{self.name} (Cards: {self.get_hand_size()})"
