{
  "targets": [{
    "target_name": "fsio",
    "conditions": [[
      "OS == \"linux\"", {
        "cflags": [
          "-Wno-unused-local-typedefs"
        ]
      }]
    ],
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    "sources": [
      "./src/poller.cc",
      "./src/fsio.cc"
    ]
  }]
}
