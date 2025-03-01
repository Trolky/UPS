import tkinter as tk
import time
from tkinter import messagebox
from player import Player
import json
import threading
import ast


class CardGameGUI(tk.Frame):
    def __init__(self, parent, controller, name: str):
        """Initializes the Card Game GUI for a single player."""
        tk.Frame.__init__(self, parent)
        self.controller = controller
        self.configure(background="green")

        # Player setup
        self.player = Player(name)
        self.previous_hand = None

        # UI components
        self.suit_buttons = []
        self.is_active = True  # Track if GUI is active
        self._setup_game_ui()

        if self.controller.network_client and self.controller.network_client.game_state:
            self.update_game_state(self.controller.network_client.game_state)

        # Networking Setup
        self.listen_thread = threading.Thread(target=self.listen_for_messages)
        self.listen_thread.daemon = True
        self.listen_thread.start()

    def _setup_game_ui(self):
        """Set up the main game UI components."""
        self.draw_pile_button = tk.Button(self, text="Draw Pile", width=10, height=5, command=self.draw_card)
        self.draw_pile_button.grid(row=0, column=0, padx=10, pady=20)

        self.discard_pile_button = tk.Button(self, text="Discard Pile", width=10, height=5)
        self.discard_pile_button.grid(row=0, column=1, padx=10, pady=20)

        self.info_frame = tk.Frame(self)
        self.info_frame.grid(row=0, column=6, rowspan=2, padx=20, pady=10)

        self.player_label = tk.Label(self.info_frame, text=f"Player: {self.player.name}")
        self.player_label.grid(row=0, column=5)

        self.deck_info_label = tk.Label(self.info_frame, text="Deck: 0 cards left")
        self.deck_info_label.grid(row=1, column=5)

        self.discard_info_label = tk.Label(self.info_frame, text="Discard Pile: 0 cards")
        self.discard_info_label.grid(row=2, column=5)

        self.label_current_player = tk.Label(self.info_frame, text="Current player: None")
        self.label_current_player.grid(row=3, column=5)

        self.log_area = tk.Text(self.info_frame, width=40, height=10, bg="lightyellow", state="disabled")
        self.log_area.grid(row=4, column=5)

    def cleanup(self):
        """Clean up resources before destroying the frame."""
        self.is_active = False
        # Clear all buttons
        for btn in self.suit_buttons:
            btn.destroy()
        self.suit_buttons = []

    def log_message(self, message: str):
        """Log messages to the log area."""
        if self.is_active and hasattr(self, 'log_area'):
            try:
                self.log_area.config(state="normal")
                self.log_area.insert(tk.END, message + "\n")
                self.log_area.see(tk.END)
                self.log_area.config(state="disabled")
            except tk.TclError:
                # Widget was destroyed, ignore the error
                pass

    def create_hand_buttons(self, hand):
        """Create buttons for Player's hand."""
        if not self.is_active:
            return

        max_columns = 4
        button_height = 5
        button_width = 10

        # Clear existing buttons first
        for btn in self.suit_buttons:
            try:
                btn.destroy()
            except tk.TclError:
                pass  # Button was already destroyed

        # Reset the button list
        self.suit_buttons = []

        for idx, card in enumerate(hand):
            current_column = idx % max_columns
            current_row = 1 + (idx // max_columns)

            # Combine suit and value for display
            card_display = f"{card['value']}{card['suit']}"

            # Create button with card details
            try:
                btn = tk.Button(self, text=card_display, width=button_width, height=button_height,
                                command=lambda c=card_display: self.play_card(c))
                btn.grid(row=current_row, column=current_column, padx=10, pady=10)
                self.suit_buttons.append(btn)
            except tk.TclError:
                pass  # Frame was destroyed, ignore the error

    def play_card(self, card: str):
        """Handles the logic when a player plays a card."""
        if self.is_active and self.controller.network_client and self.controller.network_client.connected:
            self.controller.network_client.play_card(card)

    def draw_card(self):
        """Handles drawing a card for the current player."""
        if self.is_active and self.controller.network_client and self.controller.network_client.connected:
            self.controller.network_client.draw_card()

    def listen_for_messages(self):
        """Listen for messages from the server (game state updates, etc.)."""
        while self.is_active:
            try:
                message = self.controller.network_client.get_next_message()
                if message and self.is_active:
                    # Validate message structure
                    if not isinstance(message, dict) or "type" not in message:
                        print("Received malformed message, disconnecting...")
                        self.controller.network_client.close()
                        self.controller.show_frame("MainMenu")
                        messagebox.showerror("Error", "Connection lost due to invalid message format")
                        break

                    if message["type"] == "game_state_update":
                        self.update_game_state(message)
                    elif message["type"] == "error":
                        self.log_message(message["message"])
                    elif message["type"] == "game_over":
                        mess: str = message["message"]
                        self.log_message(mess)
                        # Use after to ensure thread safety when updating UI
                        self.after(1000, lambda: self.announce_winner(mess))
                    elif message["type"] == "player_disconnected":
                        self.log_message(message["message"])
                    elif message["type"] == "player_reconnected":
                        self.log_message(message["message"])
                    elif message["type"] == "player_played_card":
                        self.log_message(message["message"])
                    elif message["type"] == "player_drawn_card":
                        self.log_message(message["message"])
                    elif message["type"] == "connect_ack":
                        pass
                    else:
                        self.controller.network_client.close()
                        self.controller.show_frame("MainMenu")
                        messagebox.showerror("Error", "Connection lost due to invalid message type")
                        break
                else:
                    time.sleep(0.05)
            except json.JSONDecodeError:
                if self.is_active:
                    self.controller.network_client.close()
                    self.controller.show_frame("MainMenu")
                    messagebox.showerror("Error", "Connection lost due to invalid message format")
                break
            except Exception as e:
                if self.is_active:
                    self.controller.network_client.close()
                    self.controller.show_frame("MainMenu")
                    messagebox.showerror("Error", f"Connection lost due to error: {str(e)}")
                break
            time.sleep(0.05)

    def update_game_state(self, game_state: dict):
        """Update the game state in the UI based on the server's response."""
        if not self.is_active:
            return

        try:
            self.deck_info_label.config(text=f"Deck: {game_state['deck_size']} cards left")
            # Update discard pile with the top card
            top_card = game_state.get('top_card')
            if isinstance(top_card, str):
                # If it's a string, attempt to parse it as a dictionary
                try:
                    top_card = ast.literal_eval(top_card)
                except (ValueError, SyntaxError):
                    top_card = {}  # Default to an empty dictionary if parsing fails
            elif not isinstance(top_card, dict):
                top_card = {}
            discard_pile_size = game_state.get("discard_pile", [])
            self.discard_info_label.config(text=f"Discard Pile: {discard_pile_size} cards")
            self.discard_pile_button.config(text=f"{top_card.get('value')}"
                                                 f"{top_card.get('suit')}")

            self.label_current_player.config(text=f"Current player: {game_state['current_player']}")

            hand = game_state.get(self.player.name, [])
            if hand != self.previous_hand:
                self.create_hand_buttons(hand)
                self.previous_hand = hand
        except tk.TclError:
            # Widget was destroyed, ignore the error
            pass

    def announce_winner(self, message: str):
        """Announce the winner and reset the game."""
        if self.is_active:
            messagebox.showinfo("Game Over", message)
            self.cleanup()
            self.controller.show_frame("MainMenu")
