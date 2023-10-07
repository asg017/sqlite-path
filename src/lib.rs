use sqlite_loadable::prelude::*;
use sqlite_loadable::{api, define_scalar_function, FunctionFlags, Result};
use std::path::Path;

pub fn path_version(context: *mut sqlite3_context, _values: &[*mut sqlite3_value]) -> Result<()> {
    api::result_text(context, format!("xv{}", env!("CARGO_PKG_VERSION")))?;
    Ok(())
}

pub fn path_debug(context: *mut sqlite3_context, _values: &[*mut sqlite3_value]) -> Result<()> {
    api::result_text(
        context,
        format!(
            "Version: v{}
Source: {}
",
            env!("CARGO_PKG_VERSION"),
            env!("GIT_HASH")
        ),
    )?;
    Ok(())
}

pub fn path_at(context: *mut sqlite3_context, values: &[*mut sqlite3_value]) -> Result<()> {
    let path = Path::new(api::value_text(values.get(0).unwrap()).unwrap());
    let idx = api::value_int64(values.get(1).unwrap());
    match path.iter().nth(idx as usize) {
        Some(p) => api::result_text(context, p.to_string_lossy())?,
        None => api::result_null(context),
    };

    Ok(())
}

#[sqlite_entrypoint]
pub fn sqlite3_path_init(db: *mut sqlite3) -> Result<()> {
    define_scalar_function(
        db,
        "path_version",
        0,
        path_version,
        FunctionFlags::DETERMINISTIC,
    )?;
    define_scalar_function(
        db,
        "path_debug",
        0,
        path_debug,
        FunctionFlags::DETERMINISTIC,
    )?;
    define_scalar_function(
        db,
        "path_at",
        2,
        path_at,
        FunctionFlags::UTF8 | FunctionFlags::DETERMINISTIC,
    )?;
    Ok(())
}
