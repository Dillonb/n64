use std::collections::HashMap;
use std::mem::offset_of;
use std::path::PathBuf;
use std::process::Command;

use jit::{r4300i_t, rsp_t};

fn parse_c_layout() -> HashMap<String, (usize, usize)> {
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

    String::from_utf8(out.stdout)
        .unwrap()
        .lines()
        .filter_map(|line| {
            let mut p = line.split_whitespace();
            let name = p.next()?;
            let offset = p.next()?.parse().ok()?;
            let size = p.next()?.parse().ok()?;
            Some((name.to_string(), (offset, size)))
        })
        .collect()
}

macro_rules! check_field {
    ($c:expr, $type:ident, $field:ident) => {{
        let key = concat!(stringify!($type), ".", stringify!($field));
        let &(c_off, c_sz) = $c.get(key).unwrap_or_else(|| panic!("{key} not in C layout"));
        let r_off = offset_of!($type, $field);
        const fn field_size<T, F>(_proj: fn(*const T) -> *const F) -> usize { std::mem::size_of::<F>() }
        let r_sz = field_size(|p: *const $type| unsafe { &raw const (*p).$field });
        assert_eq!(r_off, c_off, "{key} offset: rust={r_off} c={c_off}");
        assert_eq!(r_sz, c_sz, "{key} size: rust={r_sz} c={c_sz}");
    }};
}

#[test]
fn rsp_t_layout_matches_c() {
    let c = parse_c_layout();

    check_field!(c, rsp_t, gpr);
    check_field!(c, rsp_t, prev_pc);
    check_field!(c, rsp_t, pc);
    check_field!(c, rsp_t, next_pc);
    check_field!(c, rsp_t, sp_dmem);
    check_field!(c, rsp_t, sp_imem);
    check_field!(c, rsp_t, steps);
    check_field!(c, rsp_t, status);
    check_field!(c, rsp_t, io);
    check_field!(c, rsp_t, icache);
    check_field!(c, rsp_t, vu_regs);
    check_field!(c, rsp_t, vcc);
    check_field!(c, rsp_t, vco);
    check_field!(c, rsp_t, vce);
    check_field!(c, rsp_t, acc);
    check_field!(c, rsp_t, sync);
    check_field!(c, rsp_t, divin);
    check_field!(c, rsp_t, divin_loaded);
    check_field!(c, rsp_t, divout);
    check_field!(c, rsp_t, semaphore_held);
    check_field!(c, rsp_t, dynarec);
    check_field!(c, rsp_t, zero);
}

#[test]
fn r4300i_t_layout_matches_c() {
    let c = parse_c_layout();

    check_field!(c, r4300i_t, gpr);
    check_field!(c, r4300i_t, f);
    check_field!(c, r4300i_t, pc);
    check_field!(c, r4300i_t, next_pc);
    check_field!(c, r4300i_t, prev_pc);
    check_field!(c, r4300i_t, mult_hi);
    check_field!(c, r4300i_t, mult_lo);
    check_field!(c, r4300i_t, llbit);
    check_field!(c, r4300i_t, fcr0);
    check_field!(c, r4300i_t, fcr31);
    check_field!(c, r4300i_t, cp0);
    check_field!(c, r4300i_t, cp2_latch);
    check_field!(c, r4300i_t, icache);
    check_field!(c, r4300i_t, dcache);
    check_field!(c, r4300i_t, interrupts);
    check_field!(c, r4300i_t, branch);
    check_field!(c, r4300i_t, prev_branch);
    check_field!(c, r4300i_t, branch_likely_taken);
    check_field!(c, r4300i_t, exception);
}
