from src.main_menu import MainMenu
import tkinter as tk
from src.game_ui import CardGameGUI


class App(tk.Tk):
    def __init__(self):
        tk.Tk.__init__(self)
        self.title("Prší")
        self.network_client = None

        # Create a container frame to hold the different pages
        self.container = tk.Frame(self)
        self.container.pack(side="top", fill="both", expand=True)

        # Configure the grid to expand with the window
        self.container.grid_rowconfigure(0, weight=1)
        self.container.grid_columnconfigure(0, weight=1)

        self.frames = {}

        # Initialize and store the frames
        frame = MainMenu(self.container, self)
        self.frames["MainMenu"] = frame
        frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame("MainMenu")
        self.center_window()

    def show_frame(self, page_name: str):
        """Shows the given frame by its name."""
        if page_name in self.frames:
            frame = self.frames[page_name]
            frame.tkraise()
            self.update_idletasks()
        else:
            print(f"Error: Frame '{page_name}' not found!")

    def center_window(self):
        self.update_idletasks()
        window_width = self.winfo_reqwidth()
        window_height = self.winfo_reqheight()

        screen_width = self.winfo_screenwidth()
        screen_height = self.winfo_screenheight()

        x = (screen_width // 2) - (window_width // 2)
        y = (screen_height // 2) - (window_height // 2)

        self.geometry(f"+{x}+{y}")

    def on_closing(self):
        if self.network_client:
            self.network_client.close()
        self.destroy()

    def create_and_add_card_game_gui(self, player_name):
        """This method will create and add CardGameGUI to the frames dynamically."""
        # Remove any existing CardGameGUI frame
        if "CardGameGUI" in self.frames:
            self.frames["CardGameGUI"].destroy()
            del self.frames["CardGameGUI"]

        # Create new CardGameGUI frame
        card_game_frame = CardGameGUI(self.container, self, player_name)
        self.frames["CardGameGUI"] = card_game_frame
        card_game_frame.grid(row=0, column=0, sticky="nsew")
        self.show_frame("CardGameGUI")

