"""Barra basada en la plantilla uhr de los boletines 02 y 03."""

import time


class Progress:
    def __init__(self, total, label):
        if total <= 0:
            raise ValueError("El total de progreso debe ser positivo")
        self.total, self.label, self.actual = total, label, 0
        self.start = time.monotonic()
        self.previous_actual = None
        print("\n" + label, flush=True)
        self.show("Inicio")

    def show(self, detail):
        # https://github.com/leonardlover
        if self.previous_actual == self.actual:
            return
        width = 60
        filled = width * self.actual // self.total
        seconds = int(time.monotonic() - self.start)
        text = (f"\r[{'=' * filled}{' ' * (width - filled)}] "
                f"{100 * self.actual // self.total}%  {self.actual}/{self.total}")
        print(text, end="\n" if self.actual == self.total else "", flush=True)
        self.previous_actual = self.actual
        if self.actual == self.total:
            print(f"{self.label} | {detail} | {seconds // 60}m {seconds % 60}s", flush=True)

    def advance(self, detail):
        if self.actual == self.total:
            raise ValueError("El progreso ya esta completo")
        self.actual += 1
        self.show(detail)
