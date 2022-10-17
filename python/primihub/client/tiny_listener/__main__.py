import sys
from typing import Any

import click

import tiny_listener


def show_version(ctx: click.Context, _: Any, value: Any) -> None:
    if value and not ctx.resilient_parsing:
        click.echo(f"tiny-listener {tiny_listener.__version__}")
        ctx.exit()


@click.command()
@click.option("--app-dir", "app_dir", default=".", show_default=True, help="Your APP directory.")
@click.option(
    "--version",
    is_flag=True,
    callback=show_version,
    expose_value=False,
    is_eager=True,
    help="Display version info.",
)
@click.argument("app")
def main(app_dir: str, app: str) -> None:
    sys.path.insert(0, app_dir)
    try:
        listener: tiny_listener.Listener = tiny_listener.import_from_string(app)
    except BaseException as e:
        click.echo(e)
        sys.exit(1)
    else:
        listener.run()


if __name__ == "__main__":
    main()
