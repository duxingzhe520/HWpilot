from decimal import Decimal
from pathlib import Path

from bs4 import BeautifulSoup


STAR_VALUES = {
    "One": 1,
    "Two": 2,
    "Three": 3,
    "Four": 4,
    "Five": 5,
}


def main():
    data_dir = Path(__file__).resolve().parent / "data"
    total_price = Decimal("0")
    total_stars = 0
    longest_title = ""

    for page_no in range(1, 21):
        html_path = data_dir / f"Book_Page{page_no}.html"
        soup = BeautifulSoup(html_path.read_text(encoding="utf-8"), "html.parser")

        for book in soup.select("article.product_pod"):
            price_text = book.select_one("p.price_color").get_text(strip=True)
            total_price += Decimal(price_text.lstrip("£"))

            rating_tag = book.select_one("p.star-rating")
            rating_name = next(
                class_name for class_name in rating_tag.get("class", [])
                if class_name in STAR_VALUES
            )
            total_stars += STAR_VALUES[rating_name]

            title = book.select_one("h3 a").get("title", "")
            if len(title) > len(longest_title):
                longest_title = title

    print(f"{total_price:.2f}")
    print(total_stars)
    print(longest_title)


if __name__ == "__main__":
    main()
