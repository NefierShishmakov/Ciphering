import random


# генерируем ключ, важно чтобы он был такой же длины как открытый текст.
def generate_key(open_text_bytes_num: int) -> bytes:
    return bytes([random.randint(0, 255) for _ in range(open_text_bytes_num)])

# просто применяем алгоритм шифрования, который описан в википедии
def get_encrypted_text_in_bytes(open_text_in_bytes: bytes, key: bytes) -> bytes:
    return bytes([open_text_in_bytes[i] ^ key[i] for i in range(len(key))])

# просто применяем алгоритм дешифрования, который описан в википедии
def get_decrypted_text(encrypted_text_in_bytes: bytes, key: bytes) -> str:
    return bytes([key[i] ^ encrypted_text_in_bytes[i] for i in range(len(key))]).decode('utf-8')


def main() -> None:
    open_text: str = input("Enter the plain text: ")
    print(f"The open text is: {open_text}")

    open_text_in_bytes: bytes = open_text.encode('utf-8')
    key: bytes = generate_key(len(open_text_in_bytes))
    encrypted_text_in_bytes: bytes = get_encrypted_text_in_bytes(open_text_in_bytes, key)
    print(f"The encrypted text is: {encrypted_text_in_bytes}")

    decrypted_text: str = get_decrypted_text(encrypted_text_in_bytes, key)
    print(f"The decrypted text is: {decrypted_text}")


if __name__ == "__main__":
    main()
