; newcoffproc64.inc - NEWCOFF CodeView-aware proc64 wrapper
; Included by newcoffcv.inc after cvlocal/cv_proc_params are available.
; This file keeps automatic PROC/locals S_REGREL32 harvesting scoped to
; NEWCOFF debug builds without modifying include/macro/proc64.inc.

; harvest declarations from proc64's LOCALS block. The hook is invoked
; after the declaration line has run inside the virtual-at-localbase block,
; so the symbol already exists and cvlocal_one can derive the rsp offset.
macro cv_proc_local? line&
	match nm[count]:ty rest, line
		cvlocal_one nm, `nm, CV_T_UQUAD, 0
	else match nm:ty rest, line
		cvlocal_one nm, `nm, CV_T_UQUAD, 0
	else match nm[count] rest, line
		cvlocal_one nm, `nm, CV_T_UQUAD, 0
	else match nm rest, line
		cvlocal_one nm, `nm, CV_T_UQUAD, 0
	end match
end macro

; install the PROC interception: after the proc machinery has defined the
; parameters (and the debug prologue has opened the cvproc), each name
; from the declaration is recorded as an S_REGREL32 local automatically.
; It also shadows proc64's LOCALS block macro inside the wrapped procedure,
; preserving proc64 behavior while recording each local declaration after
; the symbol is created. The shadow is local to NEWCOFF debug procedures;
; proc64.inc itself is not modified.
macro newcoff_debug_proc64_proc? &statement&

		local procname,params,flag,regs,parmbytes,localbytes,current,parse,initlocal

		macro endp?!

				localbytes = current

				purge ret?,locals?,endl?,proclocal?

				match close:reglist, close@proc:<regs>
					close name,flag,parmbytes,localbytes,reglist
				end match

			end match
			end namespace
			end if
			end match

			purge endp?

		end macro

		calminstruction parse &args&
			local type, reg
			match procname/name args?, args, /
			jno error
			match type/name params?, args, /
			jno default
			transform type, type@proc
			jno default
			compute flag, type
			jump flag_ok
		    default:
			compute flag, 0
			arrange params, args
		    flag_ok:
			match =uses? regs/name params?, params, /
			jyes collect_regs
			arrange regs,
			exit
		    collect_regs:
			match , params
			jyes done
			match =, params, params
			jyes done
			match reg/name params?, params, /
			arrange regs, regs,reg
			jump collect_regs
		    error:
			err 'missing name'
		    done:
		end calminstruction
		parse statement

		match name, procname
		if used name
		name:
		namespace name
		outscope match local?, proclocal

			match prologue:reglist, prologue@proc:<regs>
				prologue name,flag,parmbytes,localbytes,reglist
			end match

			virtual at parmbase@proc
				namespace name
					match args, params
						iterate arg, args
							if flag and 100b
								indx %%-%+1
							end if
							match argname:type, arg
								label ?argname:type
								cvlocal_one ?argname, `argname, CV_T_UQUAD, 0
								rb type
							else
								?arg dq ?
								cvlocal_one ?arg, `arg, CV_T_UQUAD, 0
							end match
						end iterate
					end match
					parmbytes := $-(parmbase@proc)
					match p, pcountsuffix
						match base=?, name
							base#p? := parmbytes/8
						else
							name#p := parmbytes/8
						end match
					end match
				end namespace
			end virtual

			macro ret? operand
				match any, operand
					retn operand
				else
					match epilogue:reglist, epilogue@proc:<regs>
						epilogue name,flag,parmbytes,localbytes,reglist
					end match
				end match
			end macro

			current = 0

			macro initlocal
				local area,pointer,length,value
				area::
				pointer = localbase@proc+current
				length = $@ - (localbase@proc) - current
				current = $ - (localbase@proc)
				end virtual
				while length > 0
					if length < 2
						load value:byte from area:pointer
						mov byte [pointer],value
						pointer = pointer + 1
						length = length - 1
					else if length < 4
						load value:word from area:pointer
						mov word [pointer],value
						pointer = pointer + 2
						length = length - 2
					else if length < 8
						load value:dword from area:pointer
						mov dword [pointer],value
						pointer = pointer + 4
						length = length - 4
					else
						load value:qword from area:pointer
						if  value < 80000000h | value >= 1 shl 64 - 80000000h
							mov qword [pointer],value
							pointer = pointer + 8
							length = length - 8
						else
							mov dword [pointer],value and 0FFFFFFFFh
							pointer = pointer + 4
							length = length - 4
						end if
					end if
				end while
				virtual at localbase@proc+current
			end macro

			macro locals?
				namespace NEWCOFF
					CV_IN_PROC_LOCALS = 1
				end namespace
				virtual at localbase@proc+current
				iterate dword, dword,qword
					macro dword? value
						if value relativeto 0
							emit dword: value
						else
							initlocal
							local pointer
							pointer := $
							end virtual
							mov dword [pointer],value
							virtual at pointer+4
							current = $ - (localbase@proc)
						end if
					end macro
				end iterate
				macro ? line&
					esc line
					esc cv_proc_local line
					if $ > $@
						initlocal
					end if
				end macro
			end macro

			macro endl?
				purge ?, dword?,qword?
				initlocal
				end virtual
				namespace NEWCOFF
					CV_IN_PROC_LOCALS = 0
				end namespace
			end macro

			macro proclocal? args&
				locals
					iterate arg, args
						match varname[count]:type, arg
							match :size, x86.type
								?varname dbx size:count dup ?
							else
								?varname type
								repeat count - 1
									type
								end repeat
							end match
						else match varname:type, arg
							match :size, x86.type
								?varname dbx size, ?
							else
								?varname type
							end match
						else match varname[count], arg
							?varname rq count
						else match varname type, arg
							?varname type
						else
							?arg dq ?
						end match
					end iterate
				endl
			end macro
end macro

macro newcoff_debug_procs?
	macro proc?! &statement&
		newcoff_debug_proc64_proc statement
	end macro
end macro

