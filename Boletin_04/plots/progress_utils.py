"""Barra basada en la plantilla uhr de los boletines 02 y 03."""

import sys
import time


class Progress:
    def __init__(self, total, label):
        if total <= 0:
            raise ValueError("El total de progreso debe ser positivo")
        self.total, self.label, self.actual = total, label, 0
        self.start = time.monotonic()
        self.previous_width = 0
        self.show("Inicio")

    def show(self, detail):
        # https://github.com/leonardlover
        width = 30
        filled = width * self.actual // self.total
        seconds = int(time.monotonic() - self.start)
        text = (f"[{'=' * filled}{' ' * (width - filled)}] "
                f"{100 * self.actual // self.total}% {self.actual}/{self.total} | "
                f"{seconds // 60}m {seconds % 60}s | {self.label} | {detail}")
        terminal = sys.stdout.isatty()
        padding = " " * max(0, self.previous_width - len(text)) if terminal else ""
        print(("\r" if terminal else "") + text + padding,
              end="" if terminal and self.actual < self.total else "\n", flush=True)
        self.previous_width = len(text)

    def advance(self, detail):
        if self.actual == self.total:
            raise ValueError("El progreso ya esta completo")
        self.actual += 1
        self.show(detail)
