from pandas.api.types import is_integer_dtype, is_string_dtype
from primihub.MPC.psi import TwoPartyPsi, PsiType, DataType


def sample_alignment(X, id, roles, protocol: str):
    input = X[id]

    roles = roles.copy()
    host = roles["host"]
    guest = roles["guest"]
    if not isinstance(guest, list):
        guest = [guest]

    if len(guest) > 1:
        raise RuntimeError("The current PSI only support two parties.")

    guest.append(host)
    parties = guest
    receiver = host

    valid_psi_protocel = {"ECDH", "KKRT"}
    protocol = protocol.upper()
    if protocol not in valid_psi_protocel:
        raise ValueError(
            f"Invalid PSI protocol {protocol}, use {valid_psi_protocel} instead."
        )
    protocol = {
        "ECDH": PsiType.ECDH,
        "KKRT": PsiType.KKRT,
    }[protocol]

    if is_integer_dtype(input):
        data_type = DataType.Interger
    elif is_string_dtype(input):
        data_type = DataType.String
    else:
        raise RuntimeError(f"Unsupported data type: {input.dtype}")

    psi_executor = TwoPartyPsi()
    intersection = psi_executor.run(
        input=input.values,
        parties=parties,
        receiver=receiver,
        broadcast=True,
        protocol=protocol,
        data_type=data_type,
    )
    return X[input.isin(intersection)]
