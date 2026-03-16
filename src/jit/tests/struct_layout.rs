use std::collections::HashMap;
use std::mem::offset_of;
use std::path::PathBuf;
use std::process::Command;

use jit::{r4300i_t, rsp_t};

struct CField {
    offset: usize,
    size: usize,
}

struct CLayout {
    fields: HashMap<String, CField>,
    sizes: HashMap<String, usize>,
}

fn parse_c_layout() -> CLayout {
    let bin = std::env::var("DUMP_STRUCT_LAYOUT_BIN")
        .map(PathBuf::from)
        .unwrap_or_else(|_| {
            PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .ancestors()
                .nth(2)
                .unwrap()
                .join("build/dump_struct_layout")
        });
    let out = Command::new(&bin).output().expect("failed to run dump_struct_layout");
    assert!(out.status.success());

    let text = String::from_utf8(out.stdout).unwrap();
    let mut fields = HashMap::new();
    let mut sizes = HashMap::new();
    for line in text.lines() {
        let mut p = line.split_whitespace();
        let name = p.next().unwrap();
        if let Some(name) = name.strip_prefix("sizeof:") {
            sizes.insert(name.to_string(), p.next().unwrap().parse().unwrap());
        } else {
            let offset = p.next().unwrap().parse().unwrap();
            let size = p.next().unwrap().parse().unwrap();
            fields.insert(name.to_string(), CField { offset, size });
        }
    }
    CLayout { fields, sizes }
}

macro_rules! size_of_field {
    ($type:ident, $field:ident) => {{
        fn size<T, F>(_: fn(*const T) -> *const F) -> usize { std::mem::size_of::<F>() }
        size(|p: *const $type| unsafe { &raw const (*p).$field })
    }};
}

macro_rules! check_layout {
    ($c:expr, $type:ident { $($field:ident),* $(,)? }) => {{
        $({
            let key = concat!(stringify!($type), ".", stringify!($field));
            let cf = $c.fields.get(key).unwrap_or_else(|| panic!("{key} not in C layout"));
            let r_off = offset_of!($type, $field);
            let r_sz = size_of_field!($type, $field);
            assert_eq!(r_off, cf.offset, "{key} offset: rust={r_off} c={}", cf.offset);
            assert_eq!(r_sz, cf.size, "{key} size: rust={r_sz} c={}", cf.size);
        })*
        let key = stringify!($type);
        let &c_sz = $c.sizes.get(key).unwrap_or_else(|| panic!("sizeof:{key} not in C layout"));
        let r_sz = std::mem::size_of::<$type>();
        assert_eq!(r_sz, c_sz, "sizeof({key}): rust={r_sz} c={c_sz}");
    }};
}

#[test]
fn rsp_t_layout_matches_c() {
    let c = parse_c_layout();
    check_layout!(c, rsp_t {
        gpr, prev_pc, pc, next_pc, sp_dmem, sp_imem, steps, status, io,
        icache, vu_regs, vcc, vco, vce, acc, sync, divin, divin_loaded,
        divout, semaphore_held, dynarec, zero,
    });
}

#[test]
fn r4300i_t_layout_matches_c() {
    let c = parse_c_layout();
    check_layout!(c, r4300i_t {
        gpr, f, pc, next_pc, prev_pc, mult_hi, mult_lo, llbit, fcr0,
        fcr31, cp0, cp2_latch, icache, dcache, interrupts, branch,
        prev_branch, branch_likely_taken, exception,
    });
}
