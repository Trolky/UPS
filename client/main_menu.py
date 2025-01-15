import tkinter as tk
from tkinter import messagebox
from tkinter import ttk
from network_client import NetworkClient


class MainMenu(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.update_idletasks()
        self.controller = controller
        self.network_client = NetworkClient()
        self._create_widgets()
        self.check_timer = None
        self.name = None

    def connect_to_game(self):
        ip = self.ip_entry.get()
        port = self.port_entry.get()
        self.name = self.name_entry.get()

        if ip and port and self.name:
            try:
                self.network_client.connect(ip, int(port), self.name)
                # Briefly wait for connection to complete
                self.after(1000, self.check_connection)
            except Exception as e:
                messagebox.showerror("Connection Error", f"Failed to connect: {str(e)}")
        else:
            messagebox.showwarning("Input Error", "Please fill all fields.")

    def check_connection(self):
        if self.network_client.connected:
            if self.network_client.waiting_for_player:
                self.show_waiting_message()
            else:
                self.handle_successful_connection()
        else:
            # Handle error responses
            msg = self.network_client.get_next_message()
            if msg:
                if msg["type"] == "name_taken":
                    messagebox.showerror("Name Taken", "The player name is already in use. Please choose another name.")
                elif msg["type"] == "game_state_update":
                    self.handle_reconnection(msg)
                else:
                    messagebox.showerror("Connection Error", "Failed to connect to server")
            else:
                messagebox.showerror("Connection Error", "Failed to connect to server")

    def handle_successful_connection(self):
        """Handle successful connection to the server."""
        self.controller.network_client = self.network_client
        self.controller.create_and_add_card_game_gui(self.name)

    def handle_reconnection(self, game_state):
        """Handle reconnection to a previous game."""
        self.network_client.game_state = game_state  # Update client state
        messagebox.showinfo("Reconnected", "Reconnected to your previous game.")
        self.handle_successful_connection()

    def show_waiting_message(self):
        """Show waiting message if waiting for another player."""
        self.waiting_label = tk.Label(self, text="Waiting for another player...", font=("Helvetica", 14))
        self.waiting_label.grid(row=4, column=0, columnspan=2, pady=20)
        self.check_timer = self.after(1000, self.check_for_player)

    def check_for_player(self):
        """Check if another player has joined."""
        if not self.network_client.waiting_for_player:
            if self.check_timer:
                self.after_cancel(self.check_timer)
                self.check_timer = None
            if hasattr(self, 'waiting_label'):
                self.waiting_label.destroy()
            self.handle_successful_connection()
        elif self.network_client.connected:
            self.check_timer = self.after(1000, self.check_for_player)
        else:
            if hasattr(self, 'waiting_label'):
                self.waiting_label.destroy()
            if self.check_timer:
                self.after_cancel(self.check_timer)
                self.check_timer = None
            messagebox.showerror("Connection Error", "Lost connection to server")

    def on_show_frame(self):
        """Called when the frame is shown."""
        # Clean up any existing network client
        if hasattr(self.controller, 'network_client') and self.controller.network_client:
            self.controller.network_client.close()
            self.controller.network_client = None  # Clear the reference

        # Reset the network client
        self.network_client = NetworkClient()

        # Clear entry fields
        self.name_entry.delete(0, tk.END)

        # Re-enable the connect button if it was disabled
        self.connect_button.config(state="normal")

    def _create_widgets(self):
        # Set a common style for labels and entry fields
        label_font = ("Helvetica", 12)
        entry_font = ("Helvetica", 12)

        # Create and place the labels and entry fields
        tk.Label(self, text="IP:", font=label_font).grid(row=0, column=0, padx=10, pady=10, sticky="w")
        self.ip_entry = tk.Entry(self, font=entry_font)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=10, ipadx=5, ipady=5)
        self.ip_entry.insert(0, "127.0.0.1")  # Default to localhost

        tk.Label(self, text="Port:", font=label_font).grid(row=1, column=0, padx=10, pady=10, sticky="w")
        self.port_entry = tk.Entry(self, font=entry_font)
        self.port_entry.grid(row=1, column=1, padx=10, pady=10, ipadx=5, ipady=5)
        self.port_entry.insert(0, "8080")  # Default port

        tk.Label(self, text="Name:", font=label_font).grid(row=2, column=0, padx=10, pady=10, sticky="w")
        self.name_entry = tk.Entry(self, font=entry_font)
        self.name_entry.grid(row=2, column=1, padx=10, pady=10, ipadx=5, ipady=5)

        # Create Connect button
        self.connect_button = ttk.Button(self, text="Connect", command=self.connect_to_game)
        self.connect_button.grid(row=3, column=0, columnspan=2, pady=20, ipadx=1, ipady=5, sticky="ew")

        # Adjust grid column weight
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=1)
