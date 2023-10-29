import random
from io import StringIO
from PIL import Image


def convert_message_to_binary(message: str) -> str:
    return "".join(format(byte, "08b") for byte in message.encode("utf-8"))


def get_the_list_of_random_pixels_positions(img_width: int, img_height: int, binary_message_length: int) -> list[
    tuple[int, int, int]]:
    random_pixels_positions: set = set()

    # первые два числа обозначают позицию пикселя, последнее число - номер байта
    while len(random_pixels_positions) != binary_message_length:
        random_pixels_positions.add(
            (random.randint(0, img_width - 1), random.randint(0, img_height - 1), random.randint(0, 2)))

    return list(random_pixels_positions)


def hide_message_in_img(img: Image, binary_message: str, random_pixels_positions: list[tuple[int, int, int]],
                        output_img_path: str) -> None:
    sign: list[int] = [-1, 1]
    for bit, random_pixel_position in zip(binary_message, random_pixels_positions):
        position = random_pixel_position[0:2]
        pixel_rgb: list[int] = list(img.getpixel(position))
        byte_idx: int = random_pixel_position[2]
        lsb: int = pixel_rgb[byte_idx] & 1
        m = int(bit)
        s = sign[random.randint(0, 1)]
        if m == lsb:
            s = 0
        elif m != lsb and pixel_rgb[byte_idx] == 0:
            s = 1
        elif m != lsb and pixel_rgb[byte_idx] == 255:
            s = -1
        pixel_rgb[byte_idx] += s
        img.putpixel(position, tuple(pixel_rgb))
    img.save(output_img_path)


def extract_message_from_img(output_img_path: str, random_pixels_positions: list[tuple[int, int, int]]) -> str:
    img: Image = Image.open(output_img_path)
    tmp: StringIO = StringIO()

    for random_pixel_position in random_pixels_positions:
        byte_idx: int = random_pixel_position[2]
        pixel = img.getpixel(random_pixel_position[0:2])
        tmp.write(str(pixel[byte_idx] & 1))

    extracted_message_as_binary_str: str = tmp.getvalue()
    binary_data = bytes(
        int(extracted_message_as_binary_str[i:i + 8], 2) for i in range(0, len(extracted_message_as_binary_str), 8))

    return binary_data.decode("utf-8")


def main() -> None:
    message: str = input("Enter message: ")
    input_img_path: str = input("Enter input image without extension: ") + ".png"
    output_img_path: str = input("Enter output image without extension: ") + ".png"
    binary_message: str = convert_message_to_binary(message)
    binary_message_length: int = len(binary_message)
    img: Image = Image.open(input_img_path)
    img_width, img_height = img.size

    if binary_message_length > img_width * img_height:
        print(f"The message: {message} is too big to hide in image: {input_img_path}")
        return

    # получаем список рандомных позиций пикселей в изображении и задаём случайный номер байта в пикселе
    random_pixels_positions: list[tuple[int, int, int]] = get_the_list_of_random_pixels_positions(img_width, img_height,
                                                                                                  binary_message_length)
    # здесь мы сохраняем наше сообщение в другой картинке
    hide_message_in_img(img, binary_message, random_pixels_positions, output_img_path)
    # а здесь просто происходит извлечение сообщения основываясь на рандомных позициях пикселей
    extracted_message: str = extract_message_from_img(output_img_path, random_pixels_positions)

    print(f"The extracted message is {extracted_message}")


if __name__ == "__main__":
    main()

