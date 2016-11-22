{-# LANGUAGE ScopedTypeVariables #-}

module Main (main) where

import Control.Monad (mapM)
import Clingo.FFI
import Foreign
import Foreign.C.Types
import Foreign.C.String
import Foreign.Marshal.Array

show_symbol :: C'clingo_symbol_t -> IO (String)
show_symbol sym = do
    psize :: Ptr CSize <- malloc
    ret <- c'clingo_symbol_to_string_size sym psize
    size <- peek psize
    pstr :: CString <- mallocArray (fromIntegral size)
    ret <- c'clingo_symbol_to_string sym pstr size
    peekCString pstr

on_model :: Ptr C'clingo_model -> Ptr () -> Ptr CInt -> IO CInt
on_model model ptr goon = do
    psize :: Ptr CSize <- malloc
    ret <- c'clingo_model_symbols_size model c'clingo_show_type_shown psize
    size <- peek psize
    psyms :: Ptr C'clingo_symbol_t <- mallocArray (fromIntegral size)
    ret <- c'clingo_model_symbols model c'clingo_show_type_shown psyms size
    syms <- peekArray (fromIntegral size) psyms
    strs <- mapM show_symbol syms
    putStrLn $ show strs
    return 1

main = do
    (pctl :: Ptr (Ptr C'clingo_control)) <- malloc
    pargs :: Ptr CString <- malloc
    arg <- newCString "0"
    poke pargs arg
    ret <- c'clingo_control_new pargs 1 nullFunPtr nullPtr 20 pctl
    ctl <- peek pctl
    prg <- newCString "test.lp"
    ret <- c'clingo_control_load ctl prg
    base <- newCString "base"
    let part = C'clingo_part base nullPtr 0
    (ppart :: Ptr C'clingo_part) <- malloc
    poke ppart part
    ret <- c'clingo_control_ground ctl ppart 1 nullFunPtr nullPtr
    res :: Ptr C'clingo_solve_result_bitset_t <- malloc
    fun <- mk'clingo_model_callback_t on_model
    ret <- c'clingo_control_solve ctl fun nullPtr nullPtr 0 res
    return ()
