import socket
import json
import threading
import queue
import time


class NetworkClient:
    def __init__(self):
        self.socket = None
        self.message_queue = queue.Queue()
        self.running = False
        self.game_state = None
        self.connected = False
        self.waiting_for_player = False
        self.player_name = None
        self.receive_thread = None
        self.heartbeat_thread = None
        self.server_address = None
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = 10
        self.reconnect_delay = 5  # seconds
        self.game_started = False

    def _validate_message_for_state(self, message):
        """Validate if the message is appropriate for the current game state"""
        message_type = message.get("type", "")

        if self.game_started:
            # Messages that shouldn't be sent during an active game
            if message_type in ["connect", "connect_ack"]:
                return False, "Cannot send connection messages during active game"
        else:
            # Messages that shouldn't be sent before game starts
            if message_type in ["play_card", "draw_card"]:
                return False, "Cannot perform game actions before game starts"

        # Add validation for specific game states
        if message_type == "play_card" and self.waiting_for_player:
            return False, "Cannot play cards while waiting for player"

        if message_type == "draw_card" and self.waiting_for_player:
            return False, "Cannot draw cards while waiting for player"

        return True, ""

    def connect(self, ip, port, player_name):
        # Clean up any existing connection
        if self.socket:
            self.close()

        # Reset invalid message counter on new connection
        self.invalid_message_count = 0

        # Create new socket and connection
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server_address = (ip, int(port))
        self.player_name = player_name
        self.running = True
        self.connected = False
        self.waiting_for_player = False

        connect_message = {
            "type": "connect",
            "name": player_name
        }
        self.send_message(connect_message)

        # Start receiving thread
        self.receive_thread = threading.Thread(target=self._receive_messages)
        self.receive_thread.daemon = True
        self.receive_thread.start()

        # Start heartbeat thread
        self.heartbeat_thread = threading.Thread(target=self._send_heartbeat)
        self.heartbeat_thread.daemon = True
        self.heartbeat_thread.start()

    def _send_heartbeat(self):
        while self.running:
            if self.connected:
                heartbeat_message = {
                    "type": "heartbeat",
                    "name": self.player_name
                }
                self.send_message(heartbeat_message)
            time.sleep(5)  # Send heartbeat every 5 seconds

    def send_message(self, message):
        try:
            if self.socket and self.server_address:
                # Validate message for current state
                is_valid, error_msg = self._validate_message_for_state(message)
                if not is_valid:
                    self.message_queue.put({"type": "error", "message": f"Invalid message for current state: {error_msg}"})
                    return

                data = json.dumps(message, ensure_ascii=False).encode()
                self.socket.sendto(data, self.server_address)
        except Exception as e:
            print(f"Error sending message: {e}")
            self.connected = False

    def _receive_messages(self):
        while self.running:
            try:
                if not self.socket:
                    break
                data, _ = self.socket.recvfrom(4096)

                # Validate message format
                try:
                    message = json.loads(data.decode('utf-8'))
                    if not isinstance(message, dict) or "type" not in message:
                        self._handle_invalid_message("Invalid message format")
                        continue
                except json.JSONDecodeError:
                    self._handle_invalid_message("Invalid JSON format")
                    continue

                # Validate message for current state
                is_valid, error_msg = self._validate_message_for_state(message)
                if not is_valid:
                    self._handle_invalid_message(error_msg)
                    continue

                # Process valid message
                self.message_queue.put(message)

                if message["type"] == "connect_ack":
                    self.connected = True
                    self.waiting_for_player = message.get("waiting_for_player", False)
                    if not self.waiting_for_player:
                        self.game_state = message
                        self.game_started = True
                elif message["type"] == "game_state_update":
                    self.waiting_for_player = False
                    self.connected = True
                    self.game_state = message
                    self.game_started = True
                elif message["type"] == "name_taken":
                    self.connected = False
                    self.running = False
                    self.message_queue.put({"type": "name_taken"})
                elif message["type"] == "player_disconnected":
                    self.waiting_for_player = True
                elif message["type"] == "player_reconnected":
                    self.waiting_for_player = False
                    self.game_state = message
                elif message["type"] == "game_over":
                    self.connected = False
                    self.running = False
                    self.game_started = False
                elif message["type"] == "error":
                    pass
                else:
                    self._handle_invalid_message(message.get("message", "Server error"))

            except Exception as e:
                print(f"Error receiving message: {e}")
                if self.running:
                    if not self._handle_disconnect():
                        break

    def _handle_invalid_message(self, error_message):
        """Handle invalid messages and track their count"""
        print(f"Invalid message received. Disconnecting...")
        self.message_queue.put({
            "type": "unknown",
            "message": "Disconnected due to invalid message"
        })
        self.close()

    def get_next_message(self):
        try:
            return self.message_queue.get_nowait()
        except queue.Empty:
            return None

    def play_card(self, card):
        if self.waiting_for_player:
            return
        message = {
            "type": "play_card",
            "card": card,
            "player_name": self.player_name
        }
        self.send_message(message)

    def draw_card(self):
        if self.waiting_for_player:
            return
        message = {
            "type": "draw_card",
            "player_name": self.player_name
        }
        self.send_message(message)

    def close(self):
        """Properly close the connection and clean up resources."""
        self.running = False
        if self.socket:
            try:
                # Send disconnect message to server
                disconnect_message = {
                    "type": "disconnect",
                    "name": self.player_name
                }
                self.send_message(disconnect_message)
                self.socket.close()
            except:
                pass
            self.socket = None

        # Clear the message queue
        while not self.message_queue.empty():
            try:
                self.message_queue.get_nowait()
            except queue.Empty:
                break

        # Reset all client state
        self.connected = False
        self.waiting_for_player = False
        self.invalid_message_count = 0

    def _handle_disconnect(self):
        """Handle disconnection with automatic reconnection attempts."""
        if not self.running:
            return False

        while self.reconnect_attempts < self.max_reconnect_attempts and self.running:
            print(
                f"Connection lost. Attempting to reconnect ({self.reconnect_attempts + 1}/{self.max_reconnect_attempts})...")
            try:
                # Clean up socket and attempt to reconnect
                self.close()
                self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                connect_message = {
                    "type": "connect",
                    "name": self.player_name
                }
                self.send_message(connect_message)

                # Wait for server response
                try:
                    data, _ = self.socket.recvfrom(4096)
                    message = json.loads(data.decode('utf-8'))
                    if message["type"] == "connect_ack" or message["type"] == "game_state_update":
                        print("Reconnection successful!")
                        self.connected = True
                        self.reconnect_attempts = 0
                        self.game_state = message
                        self.message_queue.put(message)  # Put message in queue for UI update
                        return True
                except Exception as e:
                    print(f"Reconnect attempt failed: {e}")

            except Exception as e:
                print(f"Reconnection attempt failed: {e}")

            self.reconnect_attempts += 1
            time.sleep(self.reconnect_delay)

        if self.reconnect_attempts >= self.max_reconnect_attempts:
            print("Failed to reconnect after maximum attempts")
            self.message_queue.put({
                "type": "error",
                "message": "Connection lost and could not reconnect. Please restart the game."
            })
            self.connected = False
            return False
