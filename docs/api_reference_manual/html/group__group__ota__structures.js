var group__group__ota__structures =
[
    [ "cy_ota_storage_write_info_t", "structcy__ota__storage__write__info__t.html", [
      [ "total_size", "structcy__ota__storage__write__info__t.html#a23215e611d8933d75fc52e902d3f4502", null ],
      [ "offset", "structcy__ota__storage__write__info__t.html#a0405b3806c0de934f5f5f55ac693c184", null ],
      [ "buffer", "structcy__ota__storage__write__info__t.html#af66dba2bcbb117c85bcf20fa0357c525", null ],
      [ "size", "structcy__ota__storage__write__info__t.html#a20c92fe4a9bba891efd3319efbd05060", null ],
      [ "packet_number", "structcy__ota__storage__write__info__t.html#abe05546ecff7e5985d268248c0ebb3e5", null ],
      [ "total_packets", "structcy__ota__storage__write__info__t.html#a3d5cf26f262a52569a71a8948df60b56", null ]
    ] ],
    [ "cy_ota_storage_context_t", "structcy__ota__storage__context__t.html", [
      [ "storage_loc", "structcy__ota__storage__context__t.html#a259e6237fba59942fda5722f40a91b04", null ],
      [ "imgID", "structcy__ota__storage__context__t.html#ae60f251ef3a2366f90c377beb50fcffa", null ],
      [ "total_image_size", "structcy__ota__storage__context__t.html#a42cb555a74471636ac7e08bfabfc7386", null ],
      [ "total_bytes_written", "structcy__ota__storage__context__t.html#affd4770ac434c4a78707f1182af30f09", null ],
      [ "last_offset", "structcy__ota__storage__context__t.html#a719130f57cf7b026d9dab27bf0e70e9f", null ],
      [ "last_size", "structcy__ota__storage__context__t.html#aaa333a651ad779e0f11ffbb6141c0af1", null ],
      [ "last_packet_received", "structcy__ota__storage__context__t.html#a19c4b2aa961f28ff70d258f81e177e3a", null ],
      [ "total_packets", "structcy__ota__storage__context__t.html#a9cd89ab8a0eef15ae85ea44c7d255235", null ],
      [ "num_packets_received", "structcy__ota__storage__context__t.html#aaa92219aaeaa7dfd013a3bb625c9d2cd", null ],
      [ "last_num_packets_received", "structcy__ota__storage__context__t.html#a1d53b77a591716a65d39c2d3f77aba77", null ],
      [ "ota_is_tar_archive", "structcy__ota__storage__context__t.html#a757e5841dc9491d7b2713439a0226417", null ],
      [ "reboot_upon_completion", "structcy__ota__storage__context__t.html#a4dc6570c5b5e7123e4055388083fea81", null ],
      [ "validate_after_reboot", "structcy__ota__storage__context__t.html#aff1ac4aa0f0ac68c35d958366d9d7b29", null ]
    ] ],
    [ "cy_ota_cb_struct_t", "structcy__ota__cb__struct__t.html", [
      [ "reason", "structcy__ota__cb__struct__t.html#ae28ffd179023e8902cbd3e88785540ad", null ],
      [ "cb_arg", "structcy__ota__cb__struct__t.html#afa47e10a9fe4712558dded6625a3c484", null ],
      [ "ota_agt_state", "structcy__ota__cb__struct__t.html#afdc59f29135abdca499c3da7183df1b7", null ],
      [ "error", "structcy__ota__cb__struct__t.html#acc8a4e04f407ca0bbf0dc35e7e42f0c1", null ],
      [ "storage", "structcy__ota__cb__struct__t.html#aafd5022e59a7d19d6a103f9217710644", null ],
      [ "total_size", "structcy__ota__cb__struct__t.html#ad29859423d69a8e0815a833a70284bdb", null ],
      [ "bytes_written", "structcy__ota__cb__struct__t.html#ab43108a89dadf59dcad71d4b4c75411d", null ],
      [ "percentage", "structcy__ota__cb__struct__t.html#a1d802b856542ce6b7e59e2559026307d", null ],
      [ "connection_type", "structcy__ota__cb__struct__t.html#aabc069ca938caaeda44300df68d31903", null ]
    ] ],
    [ "cy_ota_app_info_t", "structcy__ota__app__info__t.html", [
      [ "app_id", "structcy__ota__app__info__t.html#a0c596f42b8fa1736cf1cbd7787793635", null ],
      [ "major", "structcy__ota__app__info__t.html#a7a3f1a14f143dbd2845b9fe4ff43e09e", null ],
      [ "minor", "structcy__ota__app__info__t.html#a9bc84ecaa893000ce8fd308c4807e058", null ],
      [ "build", "structcy__ota__app__info__t.html#a54e129e6fa9ae27551ce645c42a3918c", null ],
      [ "revision", "structcy__ota__app__info__t.html#a8d074972dcf1d154031b23debedf7a4e", null ],
      [ "slot", "structcy__ota__app__info__t.html#a91fb04da05781a33dc84936ed5f2f961", null ],
      [ "company_id", "structcy__ota__app__info__t.html#ae8ff214c12deb6e4ca4d2a150e107caa", null ],
      [ "product_id", "structcy__ota__app__info__t.html#af1a5cd63989e3ff89fdba351a443b425", null ]
    ] ],
    [ "cy_ota_network_params_t", "structcy__ota__network__params__t.html", [
      [ "initial_connection", "structcy__ota__network__params__t.html#abcc290be8049d49f332165ac7aafb3e0", null ],
      [ "use_get_job_flow", "structcy__ota__network__params__t.html#a5668e310a824240b5b756b23f63095c0", null ]
    ] ],
    [ "cy_ota_agent_params_t", "structcy__ota__agent__params__t.html", [
      [ "reboot_upon_completion", "structcy__ota__agent__params__t.html#adf754cf9aaa07e786e366dc2204d6429", null ],
      [ "validate_after_reboot", "structcy__ota__agent__params__t.html#a4474305852ba2b1f29e8b0d15ca671c5", null ],
      [ "do_not_send_result", "structcy__ota__agent__params__t.html#a05406d6107cc20e7cf9cde995ad3b33d", null ],
      [ "cb_func", "structcy__ota__agent__params__t.html#a2f3b68d908f64e031aca84d871554850", null ],
      [ "cb_arg", "structcy__ota__agent__params__t.html#a8734130b240a3c0121a412c1a92afffa", null ]
    ] ],
    [ "cy_ota_storage_interface_t", "structcy__ota__storage__interface__t.html", [
      [ "ota_file_open", "structcy__ota__storage__interface__t.html#a82221089aeadb8fafc41b2b296bf5b8f", null ],
      [ "ota_file_read", "structcy__ota__storage__interface__t.html#a2254932ffb585514fb9f47a861641e38", null ],
      [ "ota_file_write", "structcy__ota__storage__interface__t.html#a5c08e41b17f429a60b9fd8c2a82a088d", null ],
      [ "ota_file_close", "structcy__ota__storage__interface__t.html#a9124edbf47744ce84391554385b98fdd", null ],
      [ "ota_file_verify", "structcy__ota__storage__interface__t.html#abdb82923db46358df26c79864b54fa4c", null ],
      [ "ota_file_set_boot_pending", "structcy__ota__storage__interface__t.html#a4083fc1b8933549753c0e955a158b555", null ],
      [ "ota_file_validate", "structcy__ota__storage__interface__t.html#a32abdb8e1dc250268e41a78552042315", null ],
      [ "ota_file_get_app_info", "structcy__ota__storage__interface__t.html#a28c36929f2dd4b3649dabf0cb74ffb05", null ]
    ] ],
    [ "cy_ota_storage_read_info_t", "group__group__ota__structures.html#ga2d730f21129ca8e6841f71c351df503f", null ]
];